/**
 *	\file
 */

#pragma once

#include <ares.h>
#include <asio_cares/channel.hpp>
#include <asio_cares/detail/select.hpp>
#include <asio_cares/done.hpp>
#include <beast/core/async_result.hpp>
#include <boost/asio/handler_alloc_hook.hpp>
#include <boost/asio/handler_continuation_hook.hpp>
#include <boost/asio/handler_invoke_hook.hpp>
#include <boost/system/error_code.hpp>
#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace asio_cares {

namespace detail {

using async_process_one_signature = void (boost::system::error_code, bool);

template <typename Handler>
class async_process_one_op {
public:
	async_process_one_op () = delete;
	async_process_one_op (const async_process_one_op &) = default;
	async_process_one_op (async_process_one_op &&) = default;
	async_process_one_op & operator = (const async_process_one_op &) = default;
	async_process_one_op & operator = (async_process_one_op &&) = default;
	template <typename DeducedHandler>
	async_process_one_op (DeducedHandler && h, channel & c)
		:	inner_  (std::forward<DeducedHandler>(h)),
		    channel_(c)
	{}
	void operator () (boost::system::error_code ec, ares_socket_t readable, ares_socket_t writable) {
		if (!ec) ares_process_fd(channel_, readable, writable);
		inner_(ec, done(channel_));
	}
	void operator () () {
		inner_(boost::system::error_code{}, true);
	}
	friend void * asio_handler_allocate (std::size_t num, async_process_one_op * self) {
		assert(self);
		using boost::asio::asio_handler_allocate;
		return asio_handler_allocate(num, std::addressof(self->inner_));
	}
	friend void asio_handler_deallocate (void * ptr, std::size_t num, async_process_one_op * self) {
		assert(self);
		using boost::asio::asio_handler_deallocate;
		asio_handler_deallocate(ptr, num, std::addressof(self->inner_));
	}
	template <typename Function>
	friend void asio_handler_invoke (Function function, async_process_one_op * self) {
		assert(self);
		using boost::asio::asio_handler_invoke;
		asio_handler_invoke(std::move(function), std::addressof(self->inner_));
	}
	friend bool asio_handler_is_continuation (async_process_one_op * self) {
		assert(self);
		using boost::asio::asio_handler_is_continuation;
		return asio_handler_is_continuation(std::addressof(self->inner_));
	}
private:
	Handler   inner_;
	channel & channel_;
};

}

/**
 *	Asynchronously calls `ares_process` or
 *	`ares_process_fd` (which is actually called is
 *	an implementation detail) once.
 *
 *	It is perfectly safe to invoke this function
 *	on a \ref channel with no outstanding queries.
 *	The completion handler will simply be dispatched
 *	immediately, without error, and with its second
 *	argument set to `true`.
 *
 *	\tparam CompletionToken
 *		A type which represents the action to take
 *		upon the completion of the asynchronous
 *		operation and which determines the return
 *		value (if any) of this initiating function.
 *
 *	\param [in] c
 *		The \ref channel which shall be processed.
 *		This reference must remain valid for the
 *		lifetime of the asynchronous operation or
 *		the behavior is undefined.
 *	\param [in] token
 *		The token which encapsulates the action to
 *		take upon completion of the asynchronous
 *		operation. The completion of this asynchronous
 *		operation generates two values: The first is
 *		of type `boost::system::error_code` and
 *		represents the result of the operation. The
 *		second is the result of calling \ref done
 *		on \em c after `ares_process` or `ares_process_fd`
 *		was invoked.
 *
 *	\return
 *		Whatever is appropriate given \em CompletionToken.
 */
template <typename CompletionToken>
auto async_process_one (channel & c, CompletionToken && token) {
	beast::async_completion<CompletionToken, detail::async_process_one_signature> init(token);
	detail::async_process_one_op<beast::handler_type<CompletionToken,
		                                             detail::async_process_one_signature>> op(std::move(init.completion_handler),
														                                      c);
	if (done(c)) c.get_strand().post(std::move(op));
	else detail::async_select(c, std::move(op));
	return init.result.get();
}

}
