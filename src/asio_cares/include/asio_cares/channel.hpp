/**
 *	\file
 */

#pragma once

#include <ares.h>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/system/error_code.hpp>
#include <mpark/variant.hpp>
#include <vector>

namespace asio_cares {

/**
 *	Encapsulates a libcares channel and the state
 *	required for it to interoperate with Boost.Asio.
 */
class channel {
public:
	channel () = delete;
	channel (const channel &) = delete;
	channel (channel &&) = delete;
	channel & operator = (const channel &) = delete;
	channel & operator = (channel &&) = delete;
	/**
	 *	Creates a new channel object by calling
	 *	`ares_init`.
	 *
	 *	\param [in] ios
	 *		The `io_service` which shall be used for
	 *		asynchronous operations. This reference must
	 *		remain valid for the lifetime of the object.
	 */
	explicit channel (boost::asio::io_service & ios);
	/**
	 *	Creates a new channel object by calling
	 *	`ares_init_options`.
	 *
	 *	\param [in] options
	 *		An `ares_options` object giving the options
	 *		to pass as the second argument to `ares_init_options`.
	 *	\param [in] optmask
	 *		An integer giving the mask to pass as the
	 *		third argument to `ares_init_options`.
	 *	\param [in] ios
	 *		The `io_service` which shall be used for
	 *		asynchronous operations. This reference must
	 *		remain valid for the lifetime of the object.
	 */
	channel (const ares_options & options, int optmask, boost::asio::io_service & ios);
	/**
	 *	Cleans up a channel object.
	 */
	~channel () noexcept;
	/**
	 *	Retrieves the managed `io_service`.
	 *
	 *	\return
	 *		A reference to an `io_service` object.
	 */
	boost::asio::io_service & get_io_service () noexcept;
	/**
	 *	All operations on a channel are conceptually
	 *	operations on the underlying `ares_channel`
	 *	which are not thread safe, accordingly a
	 *	`boost::asio::strand` is used to ensure the
	 *	`ares_channel` is only accessed by one thread
	 *	of execution at a time. This method retrieves
	 *	that `boost::asio::strand` object.
	 *
	 *	\return
	 *		A reference to a `strand`.
	 */
	boost::asio::strand & get_strand () noexcept;
	/**
	 *	Operations on a channel have timeouts. This
	 *	method retrieves the `boost::asio::deadline_timer`
	 *	used to track those timeouts.
	 *
	 *	\return
	 *		A reference to a `deadline_timer`.
	 */
	boost::asio::deadline_timer & get_timer () noexcept;
	/**
	 *	The type used to represent a socket.
	 *
	 *	Note that libcares sockets may be either
	 *	UDP or TCP and therefore this value is
	 *	a variant which may be distilled to a
	 *	single value of a single type through
	 *	the use of `mpark::visit`.
	 */
	using socket_type = mpark::variant<boost::asio::ip::tcp::socket, boost::asio::ip::udp::socket>;
	/**
	 *	Retrieves a Boost.Asio socket object
	 *	corresponding to a certain libcares
	 *	socket.
	 *
	 *	If the libcares socket is not associated
	 *	with the underlying channel managed by
	 *	this object the behavior is undefined.
	 *
	 *	\param [in] socket
	 *		The socket handle from libcares.
	 *
	 *	\return
	 *		A reference to a \ref socket_type
	 *		representing the socket referred to
	 *		by \em socket.
	 */
	socket_type & get_socket (ares_socket_t socket) noexcept;
	/**
	 *	Retrieves the managed `ares_channel` object.
	 *
	 *	\return
	 *		An `ares_channel` object.
	 */
	operator ares_channel () noexcept;
private:
	using sockets_collection_type = std::vector<socket_type>;
	template <typename T>
	sockets_collection_type::iterator insertion_point (const T &) noexcept;
	template <typename T>
	sockets_collection_type::iterator find (const T &) noexcept;
	boost::asio::ip::tcp::socket tcp_socket (bool, boost::system::error_code &) noexcept;
	boost::asio::ip::udp::socket udp_socket (bool, boost::system::error_code &) noexcept;
	socket_type socket (bool, bool, boost::system::error_code &) noexcept;
	static ares_socket_t socket (int, int, int, void *) noexcept;
	static int close (ares_socket_t, void *) noexcept;
	void init () noexcept;
	ares_socket_functions       funcs_;
	ares_channel                channel_;
	boost::asio::strand         strand_;
	sockets_collection_type     sockets_;
	boost::asio::deadline_timer timer_;
};

}
