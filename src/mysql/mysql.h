#pragma once

namespace flame {
namespace mysql {
	void declare(php::extension_entry& ext);
	php::value connect(php::parameters& params);
}
}