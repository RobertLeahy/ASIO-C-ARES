#pragma once

#include <asio_cares/channel.hpp>

namespace asio_cares {
namespace tests {

void setup (channel & c);

}

namespace detail {
namespace tests {

using asio_cares::tests::setup;

}
}

}
