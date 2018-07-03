#include "../controller.h"
#include "../coroutine.h"
#include "mysql.h"
#include "_connection_base.h"
#include "_connection_pool.h"
#include "transaction.h"
#include "client.h"
#include "result.h"

namespace flame {
namespace mysql {
	void declare(php::extension_entry& ext) {
		controller_->on_init([] (const php::array& opts) {
			mysql_library_init(0, nullptr, nullptr);
		})->on_stop([] (std::exception_ptr ex) {
			mysql_library_end();
		});
		ext
			.function<connect>("flame\\mysql\\connect");
		transaction::declare(ext);
		client::declare(ext);
		result::declare(ext);
	}
	php::value connect(php::parameters& params) {
		php::object cli(php::class_entry<client>::entry());
		client* cli_ = static_cast<client*>(php::native(cli));

		cli_->c_.reset(new _connection_pool(php::parse_url(params[0])));
		std::shared_ptr<coroutine> co = coroutine::current;
		cli_->c_->exec([] (std::shared_ptr<MYSQL> c) -> MYSQL_RES* {
			return nullptr;
		}, [co, cli] (std::shared_ptr<MYSQL> c, MYSQL_RES* rs) {
			co->resume(cli);
		});
		return coroutine::async();
	}
}
}