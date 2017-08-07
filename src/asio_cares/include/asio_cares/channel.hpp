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
#include <utility>
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
private:
	using socket_type = mpark::variant<boost::asio::ip::tcp::socket, boost::asio::ip::udp::socket>;
	template <typename Function>
	static constexpr auto is_nothrow_invocable = noexcept(std::declval<Function>()(std::declval<boost::asio::ip::tcp::socket &>())) &&
	                                             noexcept(std::declval<Function>()(std::declval<boost::asio::ip::udp::socket &>()));
public:
	/**
	 *	The type used to represent a socket when
	 *	it is acquired.
	 *
	 *	In addition to making a certain socket
	 *	accessible alse releases the socket back
	 *	to the associated \ref channel once its
	 *	lifetime ends.
	 */
	class socket_guard {
	public:
		socket_guard () = delete;
		socket_guard (const socket_guard &) = delete;
		socket_guard & operator = (const socket_guard &) = delete;
		socket_guard & operator = (socket_guard &&) = delete;
		socket_guard (socket_type &, channel &) noexcept;
		socket_guard (socket_guard &&) noexcept;
		~socket_guard () noexcept;
		/**
		 *	Provides access to the underlying Boost.Asio
		 *	socket object by invoking a provided function
		 *	object with the socket as its sole argument.
		 *
		 *	Since libcares sockets may be TCP or UDP
		 *	the provided function object must accept
		 *	either `boost::asio::ip::tcp::socket` or
		 *	`boost::asio::ip::udp::socket`.
		 *
		 *	\tparam Function
		 *		The type of function object.
		 *
		 *	\param [in] function
		 *		The function object.
		 */
		template <typename Function>
		void unwrap (Function && function) noexcept(is_nothrow_invocable<Function>) {
			assert(socket_);
			mpark::visit(function, *socket_);
		}
	private:
		socket_type * socket_;
		channel *     channel_;
	};
	/**
	 *	Acquires a socket.
	 *
	 *	Acquiring a socket which has already been acquired
	 *	and not yet released results in undefined behavior.
	 *
	 *	Once a socket has been acquired it cannot be closed
	 *	by libcares until it is released. If libcares attempts
	 *	to close it while it is acquired the closure shall be
	 *	deferred until it is released.
	 *
	 *	\param [in] socket
	 *		The libcares handle for the socket to acquire.
	 *
	 *	\return
	 *		A \ref socket_guard which shall release the
	 *		socket when it goes out of scope and through
	 *		which the socket may be accessed.
	 */
	socket_guard acquire_socket (ares_socket_t socket) noexcept;
	/**
	 *	Retrieves the managed `ares_channel` object.
	 *
	 *	\return
	 *		An `ares_channel` object.
	 */
	operator ares_channel () noexcept;
	/**
	 *	Invokes a certain function object for each
	 *	socket currently in use by the channel.
	 *
	 *	Due to the fact libcares uses both TCP and
	 *	UDP sockets the provided function object must
	 *	be invocable with both `boost::asio::ip::tcp::socket`
	 *	and `boost::asio::ip::udp::socket` objects.
	 *
	 *	\tparam Function
	 *		The type of function object to invoke.
	 *
	 *	\param [in] function
	 *		The function object to invoke.
	 */
	template <typename Function>
	void for_each_socket (Function && function) noexcept(is_nothrow_invocable<Function>) {
		for (auto && state : sockets_) mpark::visit(function, state.socket);
	}
private:
	class socket_state {
	public:
		socket_state () = delete;
		socket_state (const socket_state &) = delete;
		socket_state (socket_state &&) = default;
		socket_state & operator = (const socket_state &) = delete;
		socket_state & operator = (socket_state &&) = default;
		explicit socket_state (socket_type);
		socket_type socket;
		bool        acquired;
		bool        closed;
	};
	void release_socket (int) noexcept;
	using sockets_collection_type = std::vector<socket_state>;
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
