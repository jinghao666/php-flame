#include "result_set.h"

namespace flame {
namespace mysql {
	void result_set::declare(php::extension_entry& ext) {

	}
	php::value result_set::fetch_row(php::parameters& params) {
		return nullptr;
	}
	php::value result_set::fetch_all(php::parameters& params) {
		return nullptr;
	}
}
}