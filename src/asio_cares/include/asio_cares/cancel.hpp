/**
 *	\file
 */

#pragma once

#include <asio_cares/channel.hpp>

namespace asio_cares {

/**
 *	Cancels all currently outstanding asynchronous
 *	operations on a \ref channel.
 *
 *	The completion handlers for all pending
 *	\ref async_send, \ref async_process_one, and
 *	\ref async_process operations shall be
 *	posted immediately with failures indicating
 *	that they were cancelled.
 *
 *	If this function is invoked concurrently with
 *	the completion handler of another asynchronous
 *	operation on the same channel (including
 *	intermediate handlers) the behavior is undefined.
 *	To avoid this invoke this function on the `strand`
 *	returned by \ref channel::get_strand (for information
 *	on how this avoids concurrency issues with intermediate
 *	handlers see Boost.Asio documentation for
 *	`asio_handler_invoke`).
 *
 *	\param [in] c
 *		The \ref channel whose asynchronous
 *		operations shall be cancelled. This
 *		\ref channel must remain valid until
 */
void cancel (channel & c);

}
