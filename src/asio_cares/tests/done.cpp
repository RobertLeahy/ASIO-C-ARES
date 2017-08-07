#include <asio_cares/done.hpp>

#include <asio_cares/channel.hpp>
#include <asio_cares/error.hpp>
#include <asio_cares/library.hpp>
#include <asio_cares/string.hpp>
#include <boost/asio/io_service.hpp>
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

SCENARIO("asio_cares::done correctly determines whether there are outstanding queries on an ares_channel", "[asio_cares][done]") {
	GIVEN("An ares_channel") {
		library l;
		boost::asio::io_service ios;
		channel c(ios);
		setup(c);
		THEN("asio_cares::done reports there are no outstanding queries") {
			CHECK(done(c));
		}
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
			THEN("asio_cares::done reports that there are outstanding queries") {
				CHECK_FALSE(done(c));
			}
		}
	}
}

}
}
}
