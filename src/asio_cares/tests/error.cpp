#include <asio_cares/error.hpp>

#include <ares.h>
#include <boost/system/system_error.hpp>
#include <catch.hpp>

namespace asio_cares {
namespace tests {
namespace {

SCENARIO("asio_cares::make_error_code transforms libcares error codes to std::error_code objects", "[asio_cares][error][make_error_code]") {
	GIVEN("A libcares error code") {
		int code = ARES_ENOMEM;
		WHEN("asio_cares::make_error_code is called thereupon") {
			auto ec = asio_cares::make_error_code(code);
			THEN("The contained error code is correct") {
				CHECK(ec.value() == code);
			}
			THEN("The message is correct") {
				CHECK(ec.message() == "Out of memory");
			}
		}
	}
}

SCENARIO("asio_cares::raise transforms libcares error codes which represent an error into a thrown std::system_error", "[asio_cares][error][raise]") {
	GIVEN("A libcares error code which represents an error") {
		int code = ARES_ENOMEM;
		WHEN("asio_cares::raise is called thereupon") {
			THEN("A std::system_error containing the correct std::error_code is thrown") {
				bool caught = false;
				try {
					asio_cares::raise(code);
				} catch (const boost::system::system_error & ex) {
					caught = true;
					CHECK(ex.code() == asio_cares::make_error_code(code));
				}
				CHECK(caught);
			}
		}
	}
	GIVEN("A libcares error code which does not represent an error") {
		int code = ARES_SUCCESS;
		WHEN("asio_cares::raise is called thereupon") {
			THEN("No exception is thrown") {
				CHECK_NOTHROW(asio_cares::raise(code));
			}
		}
	}
}

}
}
}
