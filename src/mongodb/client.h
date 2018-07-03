#pragma once

namespace flame {
namespace mongodb {
	class _connection_pool;
	class client: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params) { // 私有
			return nullptr;
		}
		php::value command(php::parameters& params);
	private:
		std::shared_ptr<_connection_pool> p_;
		friend php::value connect(php::parameters& params);
	};
}
}