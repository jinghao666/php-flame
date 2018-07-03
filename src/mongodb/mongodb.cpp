#include "../coroutine.h"
#include "mongodb.h"
#include "client.h"
#include "_connection_pool.h"

namespace flame {
namespace mongodb {
	void declare(php::extension_entry& ext) {
		ext
			.function<connect>("flame\\mongodb\\connect");
		client::declare(ext);
	}
	php::value connect(php::parameters& params) {
		php::object cli(php::class_entry<client>::entry());
		client* cli_ = static_cast<client*>(php::native(cli));

		cli_->p_.reset(new _connection_pool(params[0]));
		std::shared_ptr<coroutine> co = coroutine::current;
		cli_->p_->exec([] (std::shared_ptr<mongoc_client_t> c) -> std::shared_ptr<bson_t> {
			return nullptr;
		}, [co, cli] (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_t> b) {
			co->resume(cli);
		});
		return coroutine::async();
	}
}
}