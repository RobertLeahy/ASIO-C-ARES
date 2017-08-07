#include <asio_cares/send.hpp>

#include <ares.h>
#include <asio_cares/channel.hpp>
#include <asio_cares/done.hpp>
#include <asio_cares/error.hpp>
#include <asio_cares/library.hpp>
#include <asio_cares/process.hpp>
#include <asio_cares/string.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <stdexcept>
#include <catch.hpp>

#ifdef _WIN32
#include <nameser.h>
#else
#include <arpa/nameser.h>
#endif

namespace asio_cares {
namespace tests {
namespace {

SCENARIO("asio_cares::async_send may be used to submit a DNS query for asynchronous completion", "[asio_cares][send]") {
	GIVEN("An asio_cares::channel") {
		library l;
		boost::asio::io_service ios;
		channel c(ios);
		WHEN("A query is sent with asio_cares::async_send") {
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
			boost::system::error_code ec;
			bool invoked = false;
			ares_addrttl addrttls [16];
			int naddrttls = 16;
			async_send(c, ptr, buflen, [&] (auto e, auto ts, auto buf, auto len) {
				ec = e;
				invoked = true;
				REQUIRE(ares_parse_a_reply(buf, len, nullptr, addrttls, &naddrttls) == ARES_SUCCESS);
				CHECK(naddrttls > 0);
			});
			REQUIRE_FALSE(invoked);
			REQUIRE_FALSE(done(c));
			AND_WHEN("asio_cares::async_process is invoked") {
				boost::system::error_code process_error;
				bool processed = false;
				async_process(c, [&] (auto e) noexcept {
					process_error = e;
					processed = true;
				});
				ios.run();
				REQUIRE(processed);
				INFO(process_error.message());
				REQUIRE_FALSE(process_error);
				THEN("The query completes successfully") {
					REQUIRE(invoked);
					INFO(ec.message());
					CHECK_FALSE(ec);
				}
				THEN("There are no pending queries on the channel") {
					CHECK(done(c));
				}
			}
		}
		WHEN("A malformed query is sent with asio_cares::async_send") {
			boost::system::error_code ec;
			bool invoked = false;
			async_send(c, nullptr, 0, [&] (auto e, auto ts, auto, auto) noexcept {
				ec = e;
				invoked = true;
			});
			//	The whole point of this test case
			//	is to make sure that the completion
			//	handler invocation invariants are
			//	maintained even when libcares invokes
			//	within the initiating function, so
			//	if this isn't the case this test is
			//	pointless
			REQUIRE(done(c));
			THEN("The completion handler is not invoked") {
				REQUIRE_FALSE(invoked);
				AND_WHEN("asio_cares::async_process is invoked") {
					boost::system::error_code process_error;
					bool processed = false;
					async_process(c, [&] (auto e) noexcept {
						process_error = e;
						processed = true;
					});
					ios.run();
					REQUIRE(processed);
					INFO(process_error.message());
					REQUIRE_FALSE(process_error);
					THEN("The query completes with ARES_EBADQUERY") {
						REQUIRE(invoked);
						INFO(ec.message());
						CHECK(ec == make_error_code(ARES_EBADQUERY));
					}
					THEN("There are no pending queries on the channel") {
						CHECK(done(c));
					}
				}
			}
		}
		WHEN("A query is sent with asio_cares::async_send the completion handler for which throws") {
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
			async_send(c, ptr, buflen, [&] (auto, auto, auto, auto) {
				throw std::runtime_error("Test");
			});
			REQUIRE_FALSE(done(c));
			AND_WHEN("asio_cares::async_process is invoked") {
				async_process(c, [&] (auto) noexcept {});
				THEN("When boost::io_service::run is invoked the exception is thrown therefrom") {
					CHECK_THROWS_AS(ios.run(), std::runtime_error);
				}
			}
		}
	}
}

}
}
}
