#include <asio_cares/detail/select.hpp>

#include "../setup.hpp"
#include <ares.h>
#include <asio_cares/channel.hpp>
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
namespace detail {
namespace tests {
namespace {

SCENARIO("asio_cares::detail::async_select may be used to wait for readability or writability on the sockets of a libcares channel", "[asio_cares][detail][async_select]") {
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
			string s(ptr);
			bool invoked = false;
			auto f = [] (void * data, int, int, unsigned char *, int) noexcept {
				*static_cast<bool *>(data) = true;
			};
			ares_send(c, ptr, buflen, f, &invoked);
			REQUIRE_FALSE(invoked);
			AND_WHEN("asio_cares::detail::async_select is invoked") {
				bool invoked = false;
				ares_socket_t read = ARES_SOCKET_BAD;
				ares_socket_t write = ARES_SOCKET_BAD;
				boost::system::error_code ec;
				detail::async_select(c, [&] (auto e, auto r, auto w) noexcept {
					ec = e;
					read = r;
					write = w;
					invoked = true;
				});
				AND_WHEN("boost::asio::io_service::run is invoked") {
					ios.run();
					THEN("The callback is invoked") {
						REQUIRE(invoked);
						AND_THEN("No error was reported") {
							INFO(ec.message());
							REQUIRE_FALSE(ec);
							AND_THEN("At least one of the sockets is ready") {
								bool readable = read != ARES_SOCKET_BAD;
								bool writable = write != ARES_SOCKET_BAD;
								bool ready = readable || writable;
								CHECK(ready);
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
}
}
