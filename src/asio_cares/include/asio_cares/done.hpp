/**
 *	\file
 */

#pragma once

#include <ares.h>

namespace asio_cares {

/**
 *	Determines if there are any queries
 *	active on an `ares_channel`.
 *
 *	\return
 *		\em true if there are no active
 *		queries, \em false otherwise.
 */
bool done (ares_channel channel) noexcept;

}
