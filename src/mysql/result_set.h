#pragma once

namespace flame {
namespace mysql {
	class result_set: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value fetch_row(php::parameters& params);
		php::value fetch_all(php::parameters& params);
	};
}
}