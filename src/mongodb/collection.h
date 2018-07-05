#pragma once

namespace flame {
namespace mongodb {
	class _connection_pool;
	class collection: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params) { // 私有
			return nullptr;
		}
		php::value find(php::parameters& params);
	private:
		std::shared_ptr<_connection_pool> p_;
		php::string c_;
		friend class client;
	};
}
}