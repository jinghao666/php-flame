#include "core.h"
#include "os/os.h"
#include "time/time.h"
#include "log/log.h"
#include "http/http.h"
#include "redis/redis.h"
#include "rabbitmq/rabbitmq.h"

extern "C" {
	ZEND_DLEXPORT zend_module_entry* get_module() {
		static php::extension_entry ext(EXTENSION_NAME, EXTENSION_VERSION);

		php::class_entry<php::closure> class_closure("flame\\closure");
		class_closure.method<&php::closure::__invoke>("__invoke");
		ext.add(std::move(class_closure));

		ext
			.desc({"vendor/boost", BOOST_LIB_VERSION})
			.desc({"vendor/libphpext", PHPEXT_LIB_VERSION})
			.desc({"vendor/amqp-cpp", "3.1.0"});

		flame::declare(ext);
		flame::os::declare(ext);
		flame::time::declare(ext);
		flame::log::declare(ext);
		flame::http::declare(ext);
		flame::redis::declare(ext);
		flame::rabbitmq::declare(ext);
		return ext;
	}
};

