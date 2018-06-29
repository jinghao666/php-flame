#include "../controller.h"
#include "../coroutine.h"
#include "mysql.h"
#include "_connection_base.h"
#include "_connection_pool.h"
#include "transaction.h"
#include "client.h"

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
	}
	std::shared_ptr<_connection_pool> ptr;
	php::value connect(php::parameters& params) {
		std::shared_ptr<php::url> url = php::parse_url(params[0]);
		php::object cli(php::class_entry<client>::entry());
		client* cli_ = static_cast<client*>(php::native(cli));

		cli_->c_.reset(new _connection_pool(url));
		std::clog << "main: " << std::this_thread::get_id() << std::endl;
		std::shared_ptr<coroutine> co = coroutine::current;
		cli_->c_->exec(coroutine::current, [co, cli] (std::shared_ptr<coroutine> co, std::shared_ptr<MYSQL> c) {
			MYSQL* conn = c.get();
			std::clog << "help: " << std::this_thread::get_id() << std::endl;
			std::clog << "mysql connection: " << conn << " " << mysql_ping(conn) << std::endl;
			co->resume(cli);
		});
		return coroutine::async();
	}
}
}