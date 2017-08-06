#include <asio_cares/string.hpp>

#include <ares.h>
#include <utility>

namespace asio_cares {

string::string (void * ptr) noexcept
	:	ptr_(ptr)
{}

string::string (string && other) noexcept
	:	ptr_(other.ptr_)
{
	other.ptr_ = nullptr;
}

string & string::operator = (string && rhs) noexcept {
	destroy();
	using std::swap;
	swap(ptr_, rhs.ptr_);
	return *this;
}

string::~string () noexcept {
	destroy();
}

void string::destroy () noexcept {
	if (!ptr_) return;
	ares_free_string(ptr_);
	ptr_ = nullptr;
}

}
