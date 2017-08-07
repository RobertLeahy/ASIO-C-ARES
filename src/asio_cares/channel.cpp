#include <asio_cares/channel.hpp>

#include <ares.h>
#include <asio_cares/error.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/system/error_code.hpp>
#include <errno.h>
#include <mpark/variant.hpp>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>

#ifdef _WIN32
#include <ares_writev.h>
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#endif

namespace asio_cares {

static int connect (ares_socket_t fd, const struct sockaddr * addr, ares_socklen_t addr_len, void *) noexcept {
	return ::connect(fd, addr, addr_len);
}

static ares_ssize_t recvfrom (ares_socket_t fd,
	                          void * buffer,
	                          std::size_t buf_size,
	                          int flags,
	                          struct sockaddr * addr,
	                          ares_socklen_t * addr_len,
	                          void *) noexcept
{
	char * cbuffer = static_cast<char *>(buffer);
	return ::recvfrom(fd, cbuffer, buf_size, flags, addr, addr_len);
}

static ares_ssize_t sendv (ares_socket_t fd, const struct iovec * data, int len, void *) noexcept {
	#ifdef _WIN32
	ares_ssize_t retr = 0;
	for (int i = 0; i < len; ++i) {
		const struct iovec & curr = data[i];
		auto ptr = static_cast<const char *>(curr.iov_base);
		int result = ::send(fd, ptr, curr.iov_len, 0);
		if (result == SOCKET_ERROR) {
			if (retr == 0) return -1;
			return retr;
		}
		retr += result;
		if (result != curr.iov_len) return retr;
	}
	return retr;
	#else
	return ::writev(fd, data, len);
	#endif
}

channel::channel (boost::asio::io_service & ios)
	:	strand_(ios),
	    timer_ (ios)
{
	int result = ares_init(&channel_);
	raise(result);
	init();
}

channel::channel (const ares_options & options, int optmask, boost::asio::io_service & ios)
	:	strand_(ios),
		timer_ (ios)
{
	ares_options opts(options);
	int result = ares_init_options(&channel_, &opts, optmask);
	raise(result);
	init();
}

channel::~channel () noexcept {
	ares_destroy(channel_);
}

boost::asio::io_service & channel::get_io_service () noexcept {
	return strand_.get_io_service();
}

boost::asio::strand & channel::get_strand () noexcept {
	return strand_;
}

boost::asio::deadline_timer & channel::get_timer () noexcept {
	return timer_;
}

channel::socket_guard::socket_guard (socket_type & socket, channel & c) noexcept
	:	socket_ (&socket),
		channel_(&c)
{}

channel::socket_guard::socket_guard (socket_guard && other) noexcept
	:	socket_ (other.socket_),
		channel_(other.channel_)
{
	other.socket_ = nullptr;
	other.channel_ = nullptr;
}

static int get_fd (int fd) noexcept {
	return fd;
}

static int get_fd (ares_socket_t fd) noexcept {
	return int(fd);
}

template <typename... Args>
static int get_fd (const mpark::variant<Args...> & v) noexcept {
	return mpark::visit([] (const auto & socket) noexcept -> int {
		//	For some reason ::native is not const
		using no_reference_type = std::remove_reference_t<decltype(socket)>;
		using no_const_type = std::remove_const_t<no_reference_type>;
		auto & mutable_socket = const_cast<no_const_type &>(socket);
		return int(mutable_socket.native());
	}, v);
}

template <typename T>
static int get_fd (const T & state) noexcept {
	return get_fd(state.socket);
}

channel::socket_guard::~socket_guard () noexcept {
	if (socket_ && channel_) channel_->release_socket(get_fd(*socket_));
}

channel::socket_guard channel::acquire_socket (ares_socket_t socket) noexcept {
	auto retr = find(socket);
	assert(!retr->acquired);
	assert(!retr->closed);
	retr->acquired = true;
	return socket_guard(retr->socket, *this);
}

channel::operator ares_channel () noexcept {
	return channel_;
}

channel::socket_state::socket_state (socket_type socket)
	:	socket  (std::move(socket)),
		acquired(false),
		closed  (false)
{}

void channel::release_socket (int fd) noexcept {
	auto iter = find(fd);
	assert(iter->acquired);
	iter->acquired = false;
	if (iter->closed) sockets_.erase(iter);
}

template <typename T>
channel::sockets_collection_type::iterator channel::insertion_point (const T & key) noexcept {
	return std::lower_bound(
		sockets_.begin(),
		sockets_.end(),
		key,
		[] (const auto & a, const auto & b) noexcept {
			return get_fd(a) < get_fd(b);
		}
	);
}

template <typename T>
channel::sockets_collection_type::iterator channel::find (const T & key) noexcept {
	auto retr = insertion_point(key);
	assert(retr != sockets_.end());
	assert(get_fd(*retr) == get_fd(key));
	return retr;
}

boost::asio::ip::tcp::socket channel::tcp_socket (bool is_v6, boost::system::error_code & ec) noexcept {
	ec.clear();
	boost::asio::ip::tcp::socket retr(get_io_service());
	retr.open(is_v6 ? boost::asio::ip::tcp::v6() : boost::asio::ip::tcp::v4(), ec);
	return retr;
}

boost::asio::ip::udp::socket channel::udp_socket (bool is_v6, boost::system::error_code & ec) noexcept {
	ec.clear();
	boost::asio::ip::udp::socket retr(get_io_service());
	retr.open(is_v6 ? boost::asio::ip::udp::v6() : boost::asio::ip::udp::v4(), ec);
	return retr;
}

channel::socket_type channel::socket (bool udp, bool is_v6, boost::system::error_code & ec) noexcept {
	ec.clear();
	if (udp) return socket_type(udp_socket(is_v6, ec));
	return socket_type(tcp_socket(is_v6, ec));
}

ares_socket_t channel::socket (int domain, int type, int protocol, void * user_data) noexcept {
	bool is_v6 = false;
	switch (domain) {
	case AF_INET:
		break;
	case AF_INET6:
		is_v6 = true;
		break;
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
	bool udp = false;
	switch (type) {
	case SOCK_DGRAM:
		udp = true;
		break;
	case SOCK_STREAM:
		break;
	default:
		errno = EINVAL;	//	There's no ETYPENOSUPPORT
		return -1;
	}
	switch (protocol) {
	case IPPROTO_TCP:
		if (udp) {
			errno = EINVAL;
			return -1;
		}
		break;
	case IPPROTO_UDP:
		if (!udp) {
			errno = EINVAL;
			return -1;
		}
		break;
	case 0:
		break;
	default:
		errno = EPROTONOSUPPORT;
		return -1;
	}
	auto & self = *static_cast<channel *>(user_data);
	boost::system::error_code ec;
	auto socket = self.socket(udp, is_v6, ec);
	//	We blindly assume the error code is in
	//	the domain valid/meaningful for errno
	if (ec) {
		errno = ec.value();
		return -1;
	}
	ares_socket_t retr(get_fd(socket));
	auto loc = self.insertion_point(socket);
	try {
		self.sockets_.insert(loc, socket_state(std::move(socket)));
	} catch (...) {
		errno = ENOMEM;
		return -1;
	}
	errno = 0;
	return retr;
}

int channel::close (ares_socket_t fd, void * user_data) noexcept {
	auto & self = *static_cast<channel *>(user_data);
	auto iter = self.find(fd);
	assert(!iter->closed);
	iter->closed = true;
	if (!iter->acquired) self.sockets_.erase(iter);
	errno = 0;
	return 0;
}

void channel::init () noexcept {
	std::memset(&funcs_, 0, sizeof(funcs_));
	funcs_.asocket = &channel::socket;
	funcs_.aclose = &channel::close;
	funcs_.aconnect = &connect;
	funcs_.arecvfrom = &recvfrom;
	funcs_.asendv = &sendv;
	ares_set_socket_functions(channel_, &funcs_, this);
}

}
