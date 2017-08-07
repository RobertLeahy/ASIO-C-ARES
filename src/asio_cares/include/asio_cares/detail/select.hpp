/**
 *	\file
 */

#pragma once

#include "../channel.hpp"
#include <ares.h>
#include <beast/core/async_result.hpp>
#include <beast/core/handler_ptr.hpp>
#include <boost/asio/handler_alloc_hook.hpp>
#include <boost/asio/handler_continuation_hook.hpp>
#include <boost/asio/handler_invoke_hook.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <mpark/variant.hpp>
#include <cassert>
#include <cstddef>
#include <memory>
#include <new>
#include <utility>

namespace asio_cares {
namespace detail {

using async_select_signature = void (boost::system::error_code, ares_socket_t, ares_socket_t);

template <typename Handler>
class async_select_op {
private:
	class readable_tag {};
	class writable_tag {};
public:
	async_select_op () = delete;
	async_select_op (const async_select_op &) = default;
	async_select_op (async_select_op &&) = default;
	async_select_op & operator = (const async_select_op &) = default;
	async_select_op & operator = (async_select_op &&) = default;
	template <typename DeducedHandler>
	async_select_op (DeducedHandler && h, channel & c)
		:	ptr_(std::forward<DeducedHandler>(h), c)
	{}
	void operator () (boost::system::error_code ec, ares_socket_t socket, const readable_tag &) {
		common(ec);
		if (!ec && (ptr_->read == ARES_SOCKET_BAD)) ptr_->read = socket;
		upcall();
	}
	void operator () (boost::system::error_code ec, ares_socket_t socket, const writable_tag &) {
		common(ec);
		if (!ec && (ptr_->write == ARES_SOCKET_BAD)) ptr_->write = socket;
		upcall();
	}
	void operator () (boost::system::error_code ec) {
		common(ec);
		upcall();
	}
	void begin () {
		try {
			begin_impl();
		} catch (const boost::system::system_error & ex) {
			if (!ptr_->pending) throw;
			ptr_->error_code = ex.code();
			cancel();
		} catch (...) {
			//	We just deem all these errors to be out
			//	of memory errors
			if (!ptr_->pending) throw;
			ptr_->error_code = make_error_code(boost::system::errc::not_enough_memory);
			cancel();
		}
	}
	friend void * alloc_handler_allocate (std::size_t num, async_select_op * self) {
		assert(self);
		assert(self->ptr_);
		using boost::asio::asio_handler_allocate;
		return asio_handler_allocate(num, self->handler_pointer());
	}
	friend void alloc_handler_deallocate (void * ptr, std::size_t num, async_select_op * self) {
		assert(self);
		assert(self->ptr_);
		using boost::asio::asio_handler_deallocate;
		asio_handler_deallocate(ptr, num, self->handler_pointer());
	}
	template <typename Function>
	friend void asio_handler_invoke (Function function, async_select_op * self) {
		assert(self);
		assert(self->ptr_);
		using boost::asio::asio_handler_invoke;
		return asio_handler_invoke(std::move(function), self->handler_pointer());
	}
	friend bool asio_handler_is_continuation (async_select_op * self) {
		assert(self);
		assert(self->ptr_);
		if (self->ptr_->pending > 1) return true;
		using boost::asio::asio_handler_is_continuation;
		return asio_handler_is_continuation(self->handler_pointer());
	}
private:
	Handler * handler_pointer () noexcept {
		return std::addressof(ptr_.handler());
	}
	void begin_impl () {
		for (std::size_t i = 0; i < ARES_GETSOCK_MAXNUM; ++i) {
			bool readable = ARES_GETSOCK_READABLE(ptr_->flags, i);
			bool writable = ARES_GETSOCK_WRITABLE(ptr_->flags, i);
			if (!(readable || writable)) continue;
			auto ares_socket = ptr_->sockets[i];
			auto && socket = ptr_->channel.get_socket(ares_socket);
			mpark::visit([&] (auto & socket) {
				boost::asio::null_buffers buffers;
				auto && strand = ptr_->channel.get_strand();
				if (readable) {
					read_wrapper w(*this, ares_socket);
					socket.async_receive(buffers, strand.wrap(std::move(w)));
					++ptr_->pending;
				}
				if (writable) {
					write_wrapper w(*this, ares_socket);
					socket.async_send(buffers, strand.wrap(std::move(w)));
					++ptr_->pending;
				}
			}, socket);
		}
		struct timeval tv;
		if (ares_timeout(ptr_->channel, nullptr, &tv)) {
			boost::posix_time::time_duration d;
			d += boost::posix_time::seconds(tv.tv_sec);
			d += boost::posix_time::microseconds(tv.tv_usec);
			auto && timer = ptr_->channel.get_timer();
			timer.expires_from_now(d);
			timer.async_wait(ptr_->channel.get_strand().wrap(*this));
			++ptr_->pending;
		}
	}
	void common (boost::system::error_code ec) noexcept {
		assert(ptr_);
		assert(ptr_->pending > 0);
		set_error(ec);
	}
	void set_error (boost::system::error_code ec) noexcept {
		if (ptr_->completion) return;
		ptr_->completion = ec;
	}
	void upcall () {
		if (--ptr_->pending) {
			cancel();
			return;
		}
		assert(ptr_->completion);
		auto ec = *ptr_->completion;
		auto read = ptr_->read;
		auto write = ptr_->write;
		ptr_.invoke(ec, read, write);
	}
	void cancel () noexcept {
		if (ptr_->cancelled) return;
		ptr_->channel.for_each_socket([&] (auto & socket) noexcept {
			boost::system::error_code ec;
			socket.cancel(ec);
			this->set_error(ec);
		});
		boost::system::error_code ec;
		ptr_->channel.get_timer().cancel(ec);
		set_error(ec);
		ptr_->cancelled = true;
	}
	class state {
	public:
		state () = delete;
		state (const state &) = delete;
		state (state &&) = delete;
		state & operator = (const state &) = delete;
		state & operator = (state &&) = delete;
		state (const Handler &, asio_cares::channel & c)
			:	channel  (c),
				read     (ARES_SOCKET_BAD),
				write    (ARES_SOCKET_BAD),
				pending  (0),
				cancelled(false)
		{
			flags = ares_getsock(channel, sockets, ARES_GETSOCK_MAXNUM);
		}
		asio_cares::channel &                      channel;
		ares_socket_t                              read;
		ares_socket_t                              write;
		std::size_t                                pending;
		boost::optional<boost::system::error_code> completion;
		boost::system::error_code                  error_code;
		ares_socket_t                              sockets [ARES_GETSOCK_MAXNUM];
		int                                        flags;
		bool                                       cancelled;
	};
	template <typename Tag>
	class wrapper {
	public:
		wrapper () = delete;
		wrapper (const wrapper &) = default;
		wrapper (wrapper &&) = default;
		wrapper & operator = (const wrapper &) = default;
		wrapper & operator = (wrapper &&) = default;
		wrapper (async_select_op op, ares_socket_t socket) noexcept
			:	inner_ (std::move(op)),
				socket_(socket)
		{}
		template <typename... Args>
		void operator () (boost::system::error_code ec, const Args &...) {
			inner_(ec, socket_, Tag{});
		}
		friend void * asio_handler_allocate (std::size_t num, wrapper * self) {
			assert(self);
			using boost::asio::asio_handler_allocate;
			return asio_handler_allocate(num, &self->inner_);
		}
		friend void asio_handler_deallocate (void * ptr, std::size_t num, wrapper * self) {
			assert(self);
			using boost::asio::asio_handler_deallocate;
			asio_handler_deallocate(ptr, num, &self->inner_);
		}
		template <typename Function>
		friend void asio_handler_invoke (Function function, wrapper * self) {
			assert(self);
			using boost::asio::asio_handler_invoke;
			asio_handler_invoke(std::move(function), &self->inner_);
		}
		friend bool asio_handler_is_continuation (wrapper * self) {
			assert(self);
			using boost::asio::asio_handler_is_continuation;
			return asio_handler_is_continuation(&self->inner_);
		}
	private:
		async_select_op inner_;
		ares_socket_t   socket_;
	};
	using read_wrapper = wrapper<readable_tag>;
	using write_wrapper = wrapper<writable_tag>;
	using pointer = beast::handler_ptr<state, Handler>;
	pointer ptr_;
};

template <typename CompletionToken>
auto async_select (channel & c, CompletionToken && token) {
	beast::async_completion<CompletionToken, async_select_signature> init(token);
	async_select_op<beast::handler_type<CompletionToken,
		                                async_select_signature>> op(std::move(init.completion_handler), c);
	op.begin();
	return init.result.get();
}

}
}
