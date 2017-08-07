#include <asio_cares/cancel.hpp>

#include <ares.h>

namespace asio_cares {

void cancel (channel & c) {
	c.for_each_socket([] (auto & socket) {	socket.cancel();	});
	c.get_timer().cancel();
	ares_cancel(c);
}

}
