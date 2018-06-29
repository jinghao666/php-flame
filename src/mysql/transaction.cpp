#include "../coroutine.h"
#include "transaction.h"
#include "_connection_base.h"

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
	php::value transaction::query(php::parameters& params) {
		return nullptr;
	}
}
}