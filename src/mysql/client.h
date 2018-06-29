#pragma once

namespace flame {
namespace mysql {
	class client: public transaction {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params) { // 私有
			return nullptr;
		}
		php::value begin_transaction(php::parameters& params);
		// 需要代理除 commit()/rollback() 外各种 transaction 提供的方法
		inline php::value query(php::parameters& params) {
			return transaction::query(params);
		}
		friend php::value connect(php::parameters& params);
	};
}
}