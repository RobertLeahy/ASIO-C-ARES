/**
 *	\file
 */

#pragma once

#include <asio_cares/channel.hpp>
#include <asio_cares/error.hpp>
#include <beast/core/async_result.hpp>
#include <beast/core/handler_alloc.hpp>
#include <boost/asio/handler_alloc_hook.hpp>
#include <boost/asio/handler_continuation_hook.hpp>
#include <boost/asio/handler_invoke_hook.hpp>
#include <boost/system/error_code.hpp>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace asio_cares {

namespace detail {

using async_send_signature = void (boost::system::error_code, int, unsigned char *, int);

template <typename Handler>
class async_send_completion {
public:
	async_send_completion () = delete;
	async_send_completion & operator = (const async_send_completion &) = delete;
	async_send_completion & operator = (async_send_completion &&) = delete;
	async_send_completion (const async_send_completion & other)
		:	h_       (other.h_),
			ec_      (other.ec_),
			timeouts_(other.timeouts_),
			abuf_    (copy(other.abuf_, other.alen_)),
			alen_    (other.alen_)
	{}
	async_send_completion (async_send_completion && other) noexcept
		:	h_       (std::move(other.h_)),
			ec_      (other.ec_),
			timeouts_(other.timeouts_),
			abuf_    (other.abuf_),
			alen_    (other.alen_)
	{
		other.abuf_ = nullptr;
	}
	async_send_completion (Handler h, int status, int timeouts, const unsigned char * abuf, int alen) noexcept(
		std::is_nothrow_move_constructible<Handler>::value
	)	:	h_       (std::move(h)),
			ec_      (make_error_code(status)),
			timeouts_(timeouts),
			abuf_    (copy(abuf, alen)),
			alen_    (alen)
	{}
	~async_send_completion () noexcept {
		if (abuf_) std::free(abuf_);
	}
	void operator () () {
		h_(ec_, timeouts_, abuf_, alen_);
	}
	friend void * asio_handler_allocate (std::size_t num, async_send_completion * self) {
		assert(self);
		using boost::asio::asio_handler_allocate;
		return asio_handler_allocate(num, std::addressof(self->h_));
	}
	friend void asio_handler_deallocate (void * ptr, std::size_t num, async_send_completion * self) {
		assert(self);
		using boost::asio::asio_handler_deallocate;
		asio_handler_deallocate(ptr, num, std::addressof(self->h_));
	}
	template <typename Function>
	friend void asio_handler_invoke (Function function, async_send_completion * self) {
		assert(self);
		using boost::asio::asio_handler_invoke;
		asio_handler_invoke(std::move(function), std::addressof(self->h_));
	}
	friend bool asio_handler_is_continuation (async_send_completion * self) {
		assert(self);
		using boost::asio::asio_handler_is_continuation;
		return asio_handler_is_continuation(std::addressof(self->h_));
	}
private:
	static unsigned char * copy (const unsigned char * abuf, int alen) {
		if (alen == 0) return nullptr;
		auto retr = static_cast<unsigned char *>(std::malloc(alen));
		if (!retr) throw std::bad_alloc{};
		std::memcpy(retr, abuf, alen);
		return retr;
	}
	Handler                   h_;
	boost::system::error_code ec_;
	int                       timeouts_;
	unsigned char *           abuf_;
	int                       alen_;
};

template <typename Handler>
class async_send_state {
private:
	using allocator = beast::handler_alloc<async_send_state, Handler>;
	using allocator_traits = std::allocator_traits<allocator>;
	using completion_type = async_send_completion<Handler>;
public:
	async_send_state () = delete;
	async_send_state (const async_send_state &) = delete;
	async_send_state (async_send_state &&) = delete;
	async_send_state & operator = (const async_send_state &) = delete;
	async_send_state & operator = (async_send_state &&) = delete;
	static async_send_state * create (Handler h, asio_cares::channel & c) {
		allocator alloc(h);
		async_send_state * retr = allocator_traits::allocate(alloc, 1);
		try {
			new (retr) async_send_state(std::move(h), c);
		} catch (...) {
			//	It's okay for us to continue using
			//	the allocator here because the only
			//	way the constructor for this object
			//	throws is if the move constructor for
			//	Handler throws, in which case Handler
			//	wasn't moved so the allocator is still
			//	valid
			allocator_traits::deallocate(alloc, retr, 1);
			throw;
		}
		return retr;
	}
	//	Even if this throws this object is
	//	still destroyed
	void complete (int status, int timeouts, const unsigned char * abuf, int alen) {
		bool in = in_;
		auto && strand = c_.get_strand();
		auto h = free();
		completion_type completion(std::move(h), status, timeouts, abuf, alen);
		if (in) strand.post(std::move(completion));
		else strand.dispatch(std::move(completion));
	}
	void detach () noexcept {
		assert(in_);
		in_ = false;
	}
	asio_cares::channel & channel () noexcept {
		return c_;
	}
private:
	async_send_state (Handler h, asio_cares::channel & c) noexcept(
		std::is_nothrow_move_constructible<Handler>::value
	)	:	h_ (std::move(h)),
			c_ (c),
			in_(true)
	{}
	~async_send_state () = default;
	Handler free () noexcept {
		Handler retr(std::move(h_));
		allocator alloc(retr);
		this->~async_send_state();
		allocator_traits::deallocate(alloc, this, 1);
		return retr;
	}
	Handler               h_;
	asio_cares::channel & c_;
	bool                  in_;
};

template <typename Function>
void async_send_wrap (channel & c, Function && function) noexcept {
	try {
		function();
	} catch (...) {
		//	If this throws we're just done, it
		//	goes into noexcept and the process
		//	dies
		c.get_io_service().post([ex = std::current_exception()] () mutable {
			std::rethrow_exception(std::move(ex));
		});
	}
}

}

/**
 *	Invokes `ares_send` and adapts the call such
 *	that the callback invoked internally by libcares
 *	upon completion of the associated query is
 *	transmitted in accordance with the guarantees
 *	given by the Boost.Asio universal model for
 *	asynchronous operations, including (but not
 *	limited to):
 *
 *	-	The completion handler shall not be invoked
 *		from within this function even if the operation
 *		completes immediately, but shall instead be
 *		invoked later on the `strand` associated with
 *		the provided \ref channel
 *	-	The completion handler shall be invoked in a
 *		manner consistent with `asio_handler_invoke`
 *
 *	Note that unless the operation completes immediately
 *	the completion handler shall not be invoked until
 *	either:
 *
 *	-	The asynchronous operation initiated by
 *		\ref async_process runs to successful completion
 *	-	The asynchronous operation initiated by
 *		\ref async_process_one runs to successful completion
 *		and the second argument to its completion handler
 *		is `true`
 *
 *	Due to the fact it is acceptable to invoke both
 *	\ref async_process and \ref async_process_one on
 *	a \ref channel for which \ref done returns `true`
 *	callers may invoke this function and then invoke
 *	\ref async_process or \ref async_process_one
 *	without checking the edge case wherein completion
 *	occurs immediately. If desired this situation may
 *	be detected by invoking \ref done after this
 *	function returns (if \ref done returns `true` then
 *	the completion handler was posted from within this
 *	function and the invocation of \ref async_process
 *	or \ref async_process_one is technically unnecessary).
 *
 *	\tparam CompletionToken
 *		A type which represents the action to take
 *		upon the completion of the asynchronous
 *		operation and which determines the return
 *		value (if any) of this initiating function.
 *
 *	\param [in] c
 *		The \ref channel on which the query shall be
 *		sent. This reference must remain valid for the
 *		lifetime of the asynchronous operation or
 *		the behavior is undefined.
 *	\param [in] qbuf
 *		See documentation for the \em qbuf parameter
 *		of the `ares_send` function.
 *	\param [in] qlen
 *		See documentation for the \em qlen parameter
 *		of the `ares_send` function.
 *	\param [in] token
 *		The token which encapsulates the action to
 *		take upon completion of the asynchronous
 *		operation. The completion of this asynchronous
 *		operation generates four values each of which
 *		corresponds to an argument of `ares_callback`:
 *		The first argument is not applicable, the
 *		second argument is mapped to a
 *		`boost::system::error_code`, and the remaining
 *		three arguments are passed through as-is (with
 *		the exception of the fact that \em abuf shall
 *		be copied as necessary to transport the completion
 *		invocation to an appropriate context).
 *
 *	\return
 *		Whatever is appropriate given \em CompletionToken.
 */
template <typename CompletionToken>
auto async_send (channel & c, const unsigned char * qbuf, int qlen, CompletionToken && token) {
	beast::async_completion<CompletionToken, detail::async_send_signature> init(token);
	using handler_type = beast::handler_type<CompletionToken, detail::async_send_signature>;
	using state_type = detail::async_send_state<handler_type>;
	auto state = state_type::create(std::move(init.completion_handler), c);
	ares_send(c, qbuf, qlen, [] (void * arg, int status, int timeouts, unsigned char * abuf, int alen) {
		auto state = static_cast<state_type *>(arg);
		detail::async_send_wrap(state->channel(), [&] () {
			state->complete(status, timeouts, abuf, alen);
		});
	}, state);
	state->detach();
	return init.result.get();
}

}
