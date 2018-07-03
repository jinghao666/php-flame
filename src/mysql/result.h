#pragma once

namespace flame {
namespace mysql {
	class result: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		result();
		~result(); // 简单跟踪 free 过程, mysql_store_result 后跟 MYSQL 连接无关
		php::value fetch_row(php::parameters& params);
		php::value fetch_all(php::parameters& params);
	private:
		MYSQL_RES* r_;

		friend class transaction;
	};
}
}