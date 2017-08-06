/**
 *	\file
 */

#pragma once

namespace asio_cares {

/**
 *	An RAII guard for a string allocated
 *	by libcares which calls `ares_free_string`
 *	when its lifetime ends.
 */
class string {
public:
	string () = delete;
	string (const string &) = delete;
	string & operator = (const string &) = delete;
	explicit string (void * ptr) noexcept;
	string (string && other) noexcept;
	string & operator = (string && rhs) noexcept;
	~string () noexcept;
private:
	void destroy () noexcept;
	void * ptr_;
};

}
