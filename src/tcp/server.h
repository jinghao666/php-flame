#pragma once

namespace flame {
namespace tcp {
	class server: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
	};
}
}