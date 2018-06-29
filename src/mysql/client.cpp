#include "../coroutine.h"
#include "transaction.h"
#include "client.h"
#include "_connection_base.h"

namespace flame {
namespace mysql {
	void client::declare(php::extension_entry& ext) {
		php::class_entry<client> class_client("flame\\mysql\\client");
		class_client
			.method<&client::__construct>("__construct", {}, php::PRIVATE)
			.method<&client::query>("query");
		ext.add(std::move(class_client));
	}
}
}