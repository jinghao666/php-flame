#pragma once

namespace flame {
namespace tcp {
	udp::resolver* resolver_;
	void declare(php::extension_entry& ext);
	php::value connect(php::parameters& params);
}
}