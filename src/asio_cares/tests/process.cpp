#include <asio_cares/process.hpp>

#include <ares.h>
#include <asio_cares/channel.hpp>
#include <asio_cares/done.hpp>
#include <asio_cares/error.hpp>
#include <asio_cares/library.hpp>
#include <asio_cares/string.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <catch.hpp>

#ifdef _WIN32
#include <nameser.h>
#else
#include <arpa/nameser.h>
#endif

namespace asio_cares {
namespace tests {
namespace {

SCENARIO("asio_cares::async_process may be used to asynchronously complete a DNS query", "[asio_cares][process]") {
	GIVEN("An asio_cares::channel") {
		library l;
		boost::asio::io_service ios;
		channel c(ios);
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
			class state {
			public:
				state () noexcept
					:	invoked(false)
				{}
				boost::system::error_code error_code;
				bool                      invoked;
			};
			state s;
			auto f = [] (void * data, int status, int, unsigned char *, int) noexcept {
				auto && s = *static_cast<state *>(data);
				s.invoked = true;
				s.error_code = asio_cares::make_error_code(status);
			};
			ares_send(c, ptr, buflen, f, &s);
			REQUIRE_FALSE(s.invoked);
			REQUIRE_FALSE(done(c));
			WHEN("asio_cares::async_process is invoked") {
				boost::system::error_code ec;
				bool invoked = false;
				async_process(c, [&] (auto e) noexcept {
					ec = e;
					invoked = true;
				});
				ios.run();
				THEN("The query completes successfully") {
					REQUIRE(s.invoked);
					INFO(s.error_code.message());
					CHECK_FALSE(s.error_code);
				}
				THEN("There are no pending queries on the channel") {
					CHECK(done(c));
				}
			}
		}
		WHEN("asio_cares::async_process is invoked thereupon") {
			boost::system::error_code ec;
			bool invoked = false;
			async_process(c, [&] (auto e) noexcept {
				ec = e;
				invoked = true;
			});
			ios.run();
			THEN("The operation completes successfully") {
				REQUIRE(invoked);
				INFO(ec.message());
				CHECK_FALSE(ec);
			}
		}
	}
}

}
}
}
