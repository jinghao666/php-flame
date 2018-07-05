#include "../coroutine.h"
#include "transaction.h"
#include "_connection_base.h"
#include "_connection_lock.h"
#include "result.h"

namespace flame {
namespace mysql {
	void transaction::declare(php::extension_entry& ext) {
		php::class_entry<transaction> class_transaction("flame\\mysql\\transaction");
		class_transaction
			.method<&transaction::query>("query")
			.method<&transaction::commit>("commit")
			.method<&transaction::rollback>("rollback");
		ext.add(std::move(class_transaction));
	}
	php::value transaction::commit(php::parameters& params) {
		std::shared_ptr<coroutine> co = coroutine::current;
		c_->exec([] (std::shared_ptr<MYSQL> c, int& error) -> std::shared_ptr<MYSQL_RES> { // 工作线程
			MYSQL* conn = c.get();
			error = mysql_real_query(conn, "COMMIT", 6);
			return nullptr;
		}, [co] (std::shared_ptr<MYSQL> c, std::shared_ptr<MYSQL_RES> r, int error) { // 主线程
			MYSQL* conn = c.get();
			if(error) {
				co->fail(mysql_error(conn), mysql_errno(conn));
			}else{
				co->resume();
			}
		});
		return coroutine::async();
	}
	php::value transaction::rollback(php::parameters& params) {
		std::shared_ptr<coroutine> co = coroutine::current;
		c_->exec([] (std::shared_ptr<MYSQL> c, int& error) -> std::shared_ptr<MYSQL_RES> { // 工作线程
			MYSQL* conn = c.get();
			error = mysql_real_query(conn, "ROLLBACK", 8);
			return nullptr;
		}, [co] (std::shared_ptr<MYSQL> c, std::shared_ptr<MYSQL_RES> r, int error) { // 主线程
			MYSQL* conn = c.get();
			if(error) {
				co->fail(mysql_error(conn), mysql_errno(conn));
			}else{
				co->resume();
			}
		});
		return coroutine::async();
	}
	php::value transaction::escape(php::parameters& params) {
		php::buffer b;
		c_->escape(b, params[0]);
		return std::move(b);
	}
	php::value transaction::query(php::parameters& params) {
		c_->query(coroutine::current, php::object(this), params[0]);
		return coroutine::async();
	}
}
}