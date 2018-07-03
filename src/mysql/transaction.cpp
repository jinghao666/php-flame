#include "../coroutine.h"
#include "transaction.h"
#include "_connection_base.h"
#include "result.h"

namespace flame {
namespace mysql {
	void transaction::declare(php::extension_entry& ext) {
		php::class_entry<transaction> class_transaction("flame\\mysql\\transaction");
		class_transaction
			.method<&transaction::query>("query");
		ext.add(std::move(class_transaction));
	}
	php::value transaction::commit(php::parameters& params) {
		return nullptr;
	}
	php::value transaction::rollback(php::parameters& params) {
		return nullptr;
	}
	php::value transaction::escape(php::parameters& params) {
		php::buffer b;
		c_->escape(b, params[0]);
		return std::move(b);
	}
	php::value transaction::query(php::parameters& params) {
		php::object ref(this);
		php::string str = params[0]; // PHP 对象必须在回到主线程后销毁
		std::shared_ptr<coroutine> co = coroutine::current;
		// 执行"异步"操作
		std::shared_ptr<_connection_base> cc = c_;
		c_->exec([cc, co, str] (std::shared_ptr<MYSQL> c) -> MYSQL_RES* { // 工作线程
			MYSQL* conn = c.get();
			int error = mysql_real_query(conn, str.c_str(), str.size());
			if(error != 0) return nullptr;
			MYSQL_RES* r = mysql_store_result(conn);
			if(r) return r;
			if(mysql_field_count(conn) == 0) {
				return (MYSQL_RES*) conn; // 特殊标记返回
			}
			return nullptr;
		}, [cc, co, str] (std::shared_ptr<MYSQL> c, MYSQL_RES* r) { // 主线程
			MYSQL* conn = c.get();
			if(r == nullptr) { // 错误
				co->fail(mysql_error(conn), mysql_errno(conn));
			}else{
				php::object rs(php::class_entry<result>::entry());
				rs.set("affected_rows", static_cast<std::int64_t>(mysql_affected_rows(conn)));
				rs.set("insert_id", static_cast<std::int64_t>(mysql_insert_id(conn)));
				if(conn != (MYSQL*)r) { // 标记包含 ResultSet 数据的情况
					result* rs_ = static_cast<result*>(php::native(rs));
					rs_->r_ = r;
				}
				co->resume(rs);
			}
		});
		return coroutine::async();
	}
}
}