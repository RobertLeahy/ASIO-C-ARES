#include <asio_cares/library.hpp>

#include <ares.h>
#include <asio_cares/error.hpp>

namespace asio_cares {

library::library (int flags) {
	int result = ares_library_init(flags);
	raise(result);
}

library::~library () noexcept {
	ares_library_cleanup();
}

}
