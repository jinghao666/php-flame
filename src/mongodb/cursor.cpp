#include "../coroutine.h"
#include "cursor.h"
#include "_connection_lock.h"
#include "mongodb.h"

namespace flame {
namespace mongodb {
	void cursor::declare(php::extension_entry& ext) {
		php::class_entry<cursor> class_cursor("flame\\mongodb\\cursor");
		class_cursor
			.method<&cursor::__construct>("__construct", {}, php::PRIVATE)
			.method<&cursor::next>("next")
			.method<&cursor::to_array>("__toArray");
		ext.add(std::move(class_cursor));
	}
	php::value cursor::next(php::parameters& params) {
		std::shared_ptr<coroutine> co = coroutine::current;
		php::object ref(this);
		p_->exec([this] (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_error_t> error) -> std::shared_ptr<bson_t> {
			mongoc_cursor_t* o = c_.get();
			const bson_t* doc;
			if(!mongoc_cursor_next(o, &doc)) {
				bson_error_t* err = new bson_error_t;
				if(mongoc_cursor_error(o, err)) {
					error.reset(err);
				}else{
					delete err;
				}
			}
			return std::shared_ptr<bson_t>(const_cast<bson_t*>(doc), boost::null_deleter());
		}, [this, co, ref] (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_t> b, std::shared_ptr<bson_error_t> error) {
			if(error) {
				co->fail(error->message, error->code);
			}else{
				co->resume(convert(b));
			}
		});
		return coroutine::async();
	}
	php::value cursor::to_array(php::parameters& params) {
		std::shared_ptr<coroutine> co = coroutine::current;
		php::object ref(this);
		p_->exec([this] (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_error_t> error) -> std::shared_ptr<bson_t> {
			mongoc_cursor_t* o = c_.get();
			bson_t* docs = bson_new();
			const bson_t* doc;
			int i = 0;
			while(mongoc_cursor_next(o, &doc)) {
				std::string key = std::to_string(i++);
				bson_append_document(docs, key.c_str(), key.size(), doc);
			}
			bson_error_t* err = new bson_error_t;
			if(mongoc_cursor_error(o, err)) {
				error.reset(err);
			}else{
				delete err;
			}
			return std::shared_ptr<bson_t>(docs, bson_destroy);
		}, [this, co, ref] (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_t> b, std::shared_ptr<bson_error_t> error) {
			if(error) {
				co->fail(error->message, error->code);
			}else{
				co->resume(convert(b));
			}
		});
		return coroutine::async();
	}
}
}