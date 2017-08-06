/**
 *	\file
 */

#pragma once

#include <ares.h>

namespace asio_cares {

/**
 *	Calls `ares_library_init` on construction
 *	and `ares_library_cleanup` on destruction.
 */
class library {
public:
	library (const library &) = delete;
	library (library &&) = delete;
	library & operator = (const library &) = delete;
	library & operator = (library &&) = delete;
	/**
	 *	Calls `ares_library_init`.
	 *
	 *	\param [in] flags
	 *		The \em flags parameter to pass through
	 *		to `ares_library_init`. Defaults to
	 *		`ARES_LIB_INIT_ALL`.
	 */
	explicit library (int flags = ARES_LIB_INIT_ALL);
	/**
	 *	Calls `ares_library_cleanup`.
	 */
	~library () noexcept;
};

}
