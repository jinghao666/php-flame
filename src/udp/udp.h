#pragma once

namespace flame {
namespace udp {
	extern boost::asio::ip::udp::resolver* resolver_;
	void declare(php::extension_entry& ext);
	php::value connect(php::parameters& params);
}
}