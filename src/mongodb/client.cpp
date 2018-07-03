#include "../coroutine.h"
#include "client.h"
#include "_connection_pool.h"

namespace flame {
namespace mongodb {
	void client::declare(php::extension_entry& ext) {
		php::class_entry<client> class_client("flame\\mongodb\\client");
		class_client
			.method<&client::__construct>("__construct", {}, php::PRIVATE)
			.method<&client::command>("command");
		ext.add(std::move(class_client));
	}
	php::value client::command(php::parameters& params) {
		return nullptr;
	}
}
}