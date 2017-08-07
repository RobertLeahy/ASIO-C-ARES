#include <asio_cares/done.hpp>

#include <ares.h>

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace asio_cares {

bool done (ares_channel channel) noexcept {
	fd_set a;
	FD_ZERO(&a);
	fd_set b;
	FD_ZERO(&b);
	return ares_fds(channel, &a, &b) == 0;
}

}
