#include "../coroutine.h"
#include "collection.h"
#include "_connection_base.h"
#include "_connection_pool.h"
#include "_connection_lock.h"
#include "mongodb.h"
#include "cursor.h"
#include "collection.h"

namespace flame {
namespace mongodb {
	void collection::declare(php::extension_entry& ext) {
		php::class_entry<collection> class_collection("flame\\mongodb\\collection");
		class_collection
			.method<&collection::__construct>("__construct", {}, php::PRIVATE)
			.method<&collection::find>("find", {
				{"filter", php::TYPE::ARRAY},
				{"projection", php::TYPE::ARRAY, false, true},
				{"sort", php::TYPE::ARRAY, false, true},
				{"limit", php::TYPE::ARRAY, false, true},
			});
		ext.add(std::move(class_collection));
	}
	php::value collection::find(php::parameters& params) {
		return nullptr;
	}
}
}