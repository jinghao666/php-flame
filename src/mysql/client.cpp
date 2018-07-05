#include "../coroutine.h"
#include "transaction.h"
#include "client.h"
#include "_connection_base.h"
#include "_connection_pool.h"
#include "_connection_lock.h"
#include "mysql.h"

namespace flame {
namespace mysql {
	void client::declare(php::extension_entry& ext) {
		php::class_entry<client> class_client("flame\\mysql\\client");
		class_client
			.method<&client::__construct>("__construct", {}, php::PRIVATE)
			.method<&client::begin_tx>("begin_tx")
			.method<&client::query>("query")
			.method<&client::select>("select", {
				{"table", php::TYPE::STRING},
				{"fields", php::TYPE::UNDEFINED},
				{"where", php::TYPE::UNDEFINED},
				{"order", php::TYPE::UNDEFINED, false, true},
				{"limit", php::TYPE::UNDEFINED, false, true},
			});
		ext.add(std::move(class_client));
	}
	php::value client::begin_tx(php::parameters& params) {
		std::shared_ptr<coroutine> co = coroutine::current;
		c_->exec([] (std::shared_ptr<MYSQL> c, int& error) -> std::shared_ptr<MYSQL_RES> { // 工作线程
			MYSQL* conn = c.get();
			error = mysql_real_query(conn, "START TRANSACTION", 17);
			return nullptr;
		}, [co] (std::shared_ptr<MYSQL> c, std::shared_ptr<MYSQL_RES> r, int error) { // 主线程
			MYSQL* conn = c.get();
			if(error) {
				co->fail(mysql_error(conn), mysql_errno(conn));
			}else{
				php::object tx(php::class_entry<transaction>::entry());
				transaction* tx_ = static_cast<transaction*>(php::native(tx));
				tx_->c_.reset(new _connection_lock(c)); // 继续持有当前连接
				co->resume(std::move(tx));
			}
		});
		return coroutine::async();
	}
	php::value client::query(php::parameters& params) {
		c_->query(coroutine::current, php::object(this), params[0]);
		return coroutine::async();
	}
	php::value client::select(php::parameters& params) {
		php::buffer buf;
		build_where(c_, buf, params[2]);
		return std::move(buf);
	}
}
}