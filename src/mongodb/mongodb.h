#pragma once

namespace flame {
namespace mongodb {
	void declare(php::extension_entry& ext);
	php::value connect(php::parameters& params);
}
}