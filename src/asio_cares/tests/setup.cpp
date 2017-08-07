#include "setup.hpp"

#include <ares.h>
#include <asio_cares/error.hpp>
#include <boost/endian/conversion.hpp>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif

namespace asio_cares {
namespace tests {

void setup (channel & c) {
	ares_addr_node node;
	std::memset(&node, 0, sizeof(node));
	uint32_t addr = 4;
	addr <<= 8;
	addr |= 2;
	addr <<= 8;
	addr |= 2;
	addr <<= 8;
	addr |= 1;
	static_assert(sizeof(addr) == sizeof(node.addr.addr4), "Sizes not equal");
	boost::endian::native_to_big_inplace(addr);
	std::memcpy(&node.addr.addr4, &addr, sizeof(addr));
	node.family = AF_INET;
	int result = ares_set_servers(c, &node);
	raise(result);
}

}
}
