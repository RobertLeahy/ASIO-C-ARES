#include <asio_cares/cancel.hpp>

#include <ares.h>
#include <asio_cares/channel.hpp>
#include <asio_cares/done.hpp>
#include <asio_cares/error.hpp>
#include <asio_cares/library.hpp>
#include <asio_cares/process.hpp>
#include <asio_cares/send.hpp>
#include <asio_cares/string.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/error.hpp>
#include <boost/system/error_code.hpp>
#include "setup.hpp"
#include <catch.hpp>

#ifdef _WIN32
#include <nameser.h>
#else
#include <arpa/nameser.h>
#endif

namespace asio_cares {
namespace tests {
namespace {

SCENARIO("asio_cares::cancel may be used to cancel outstanding asynchronous operations", "[asio_cares][cancel]") {
	GIVEN("An asio_cares::channel") {
		library l;
		boost::asio::io_service ios;
		channel c(ios);
		setup(c);
		WHEN("A query is sent thereupon") {
			unsigned char * ptr;
			int buflen;
			int result = ares_create_query("google.com",
				                           ns_c_any,
				                           ns_t_a,
				                           0,
				                           1,
				                           &ptr,
				                           &buflen,
				                           0);
			raise(result);
			string g(ptr);
			bool invoked = false;
			boost::system::error_code ec;
			async_send(c, ptr, buflen, [&] (auto e, auto, auto, auto) noexcept {
				ec = e;
				invoked = true;
			});
			REQUIRE_FALSE(invoked);
			REQUIRE_FALSE(done(c));
			AND_WHEN("asio_cares::async_process is invoked") {
				bool process_invoked = false;
				boost::system::error_code process_ec;
				async_process(c, [&] (auto e) noexcept {
					process_ec = e;
					process_invoked = true;
				});
				REQUIRE_FALSE(process_invoked);
				AND_WHEN("asio_cares::cancel is invoked") {
					cancel(c);
					AND_WHEN("boost::asio::io_service::run is invoked") {
						ios.run();
						THEN("Both pending operations are cancelled") {
							REQUIRE(invoked);
							INFO(ec.message());
							bool cancelled = ec == make_error_code(ARES_ECANCELLED);
							bool ok = !ec;
							bool ok_or_cancelled = ok || cancelled;
							CHECK(ok_or_cancelled);
							REQUIRE(process_invoked);
							INFO(process_ec.message());
							cancelled = process_ec == make_error_code(boost::asio::error::operation_aborted);
							ok = !process_ec;
							ok_or_cancelled = ok || cancelled;
							CHECK(ok_or_cancelled);
						}
					}
				}
			}
		}
	}
}

}
}
}
