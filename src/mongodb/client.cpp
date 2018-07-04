#include "../coroutine.h"
#include "client.h"
#include "_connection_pool.h"
#include "_connection_lock.h"
#include "mongodb.h"
#include "cursor.h"

namespace flame {
namespace mongodb {
	void client::declare(php::extension_entry& ext) {
		php::class_entry<client> class_client("flame\\mongodb\\client");
		class_client
			.method<&client::__construct>("__construct", {}, php::PRIVATE)
			.method<&client::command>("command", {
				{"command", php::TYPE::ARRAY}
			});
		ext.add(std::move(class_client));
	}
	php::value client::command(php::parameters& params) {
		std::shared_ptr<bson_t> cmd = convert(params[0]);
		std::shared_ptr<coroutine> co = coroutine::current;
		p_->exec([cmd] (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_error_t> error) -> std::shared_ptr<bson_t> {
			mongoc_client_t* cli = c.get();
			const mongoc_uri_t *uri = mongoc_client_get_uri(cli);

			bson_t*       rpl = bson_new();
			bson_error_t* err = new bson_error_t();
			if(!mongoc_client_command_with_opts(cli, mongoc_uri_get_database(uri), cmd.get(), mongoc_uri_get_read_prefs_t(uri), nullptr, rpl, err)) {

				error.reset(err);
			}
			// 返回 reply 必须进行 bson_destroy 释放
			return std::shared_ptr<bson_t>(rpl, bson_destroy);
		}, [co] (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_t> b, std::shared_ptr<bson_error_t> error) {
			if(error) {
				co->fail(error->message, error->code);
				return;
			}
			if(bson_has_field(b.get(), "cursor")) {
				php::object o(php::class_entry<cursor>::entry());
				cursor* o_ = static_cast<cursor*>(php::native(o));
				o_->p_.reset(new _connection_lock(c)); // 继续持有当前客户端指针 (不释放)
				o_->c_.reset(mongoc_cursor_new_from_command_reply_with_opts(c.get(), b.get(), nullptr), mongoc_cursor_destroy);
				co->resume(std::move(o));
			}else{
				co->resume(convert(b));
			}
		});
		return coroutine::async();
	}
}
}