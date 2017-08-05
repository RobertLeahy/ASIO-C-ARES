#include <asio_cares/error.hpp>

#include <ares.h>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <string>

namespace asio_cares {

boost::system::error_code make_error_code (int code) noexcept {
	static const class : public boost::system::error_category {
	public:
		const char * name () const noexcept override {
			return "C-Ares";
		}
		std::string message (int code) const override {
			return ares_strerror(code);
		}
	} category;
	return boost::system::error_code(code, category);
}

void raise (int code) {
	auto ec = make_error_code(code);
	if (ec) throw boost::system::system_error(ec);
}

}
