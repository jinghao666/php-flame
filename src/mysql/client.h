#pragma once

namespace flame {
namespace mysql {
	class _connection_pool;
	class client: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params) { // 私有
			return nullptr;
		}
		php::value begin_tx(php::parameters& params);
		php::value query(php::parameters& params);
		php::value select(php::parameters& params);
	protected:
		std::shared_ptr<_connection_pool> c_;
		friend php::value connect(php::parameters& params);
	};
}
}