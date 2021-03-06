#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "client.h"
#include "collection_implement.h"
#include "collection.h"
#include "mongodb.h"
#include "cursor_implement.h"
#include "cursor.h"
#include "write_result.h"


namespace flame {
namespace db {
namespace mongodb {
	void collection::init(thread_worker* worker, client* cli, mongoc_collection_t* col) {
		impl = new collection_implement(worker, this, col);
		ref_ = php::object(cli);
	}
	php::value collection::__destruct(php::parameters& params) {
		collection_request_t* ctx = new collection_request_t {
			nullptr, impl, nullptr
		};
		ctx->req.data = ctx;
		// 这里不能使用 close_work ，实际关闭在 client 中
		impl->worker_->queue_work(&ctx->req, collection_implement::close_wk, default_cb);
		return nullptr;
	}
	void collection::default_cb(uv_work_t* w, int status) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		if(ctx->rv.is_pointer()) {
			collection_response_t* res = ctx->rv.ptr<collection_response_t>();
			if(res->type == RETURN_VALUE_TYPE_ERROR) {
				ctx->rv = php::exception::create(res->error.message, res->error.code);
			}else if(res->type == RETURN_VALUE_TYPE_REPLY) {
				ctx->rv = write_result::create_from(&res->reply);
			}
			delete res;
		}
		if(ctx->co) ctx->co->next(ctx->rv);
		delete ctx;
	}
	php::value collection::count(php::parameters& params) {
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr
		};
		ctx->req.data = ctx;
		if(params.length() > 0) {
			php::array& filter = static_cast<php::array&>(params[0]);
			if(filter.is_array()) {
				ctx->doc1 = filter;
			}
		}
		if(params.length() > 1) {
			php::array& opts = static_cast<php::array&>(params[1]);
			if(opts.is_array()) {
				ctx->doc3 = opts;
			}
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::count_wk, default_cb);
		return flame::async(this);
	}
	php::value collection::insert_one(php::parameters& params) {
		php::array& doc = static_cast<php::array&>(params[0]);
		if(!doc.is_array() ) {
			throw php::exception("illegal document, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, doc
		};
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::insert_one_wk, default_cb);
		return flame::async(this);
	}
	php::value collection::insert_many(php::parameters& params) {
		php::array& docs = static_cast<php::array&>(params[0]);
		if(!docs.is_array()) {
			throw php::exception("illegal document, document array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, docs, php::array(nullptr)
		};
		ctx->req.data = ctx;
		if(params.length() > 1) {
			php::array& opts = static_cast<php::array&>(params[1]);
			if(opts.is_array()) {
				ctx->doc3 = opts;
			}
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::insert_many_wk, default_cb);
		return flame::async(this);
	}
	php::value collection::remove_one(php::parameters& params) {
		php::array& filter = static_cast<php::array&>(params[0]);
		if(!filter.is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter, php::array(nullptr)
		};
		if(params.length() > 1) {
			php::array& opts = static_cast<php::array&>(params[1]);
			if(opts.is_array()) {
				ctx->doc3 = opts;
			}
		}
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::remove_one_wk, default_cb);
		return flame::async(this);
	}
	php::value collection::remove_many(php::parameters& params) {
		php::array& filter = static_cast<php::array&>(params[0]);
		if(!filter.is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter, php::array(nullptr),
		};
		if(params.length() > 1) {
			php::array& opts = static_cast<php::array&>(params[1]);
			if(opts.is_array()) {
				ctx->doc3 = opts;
			}
		}
		ctx->req.data = ctx;
		impl->worker_->queue_work(&ctx->req, collection_implement::remove_many_wk, default_cb);
		return flame::async(this);
	}
	php::value collection::update_one(php::parameters& params) {
		php::array& filter = static_cast<php::array&>(params[0]);
		php::array& update = static_cast<php::array&>(params[1]);
		if(!filter.is_array() || !update.is_array()) {
			throw php::exception("illegal filter / update, associate array is required");
		}

		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter, update, php::array(nullptr),
		};
		ctx->req.data = ctx;
		if(params.length() > 2) {
			if(params[2].is_true()) {
				ctx->doc3 = php::array(1);	
				ctx->doc3.at("upsert",6) = php::BOOL_TRUE;
			}else if(params[2].is_array()) {
				ctx->doc3 = static_cast<php::array&>(params[2]);
			}
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::update_one_wk, default_cb);
		return flame::async(this);
	}
	php::value collection::update_many(php::parameters& params) {
		php::array& filter = static_cast<php::array&>(params[0]);
		php::array& update = static_cast<php::array&>(params[1]);
		if(!filter.is_array() || !update.is_array()) {
			throw php::exception("illegal filter / update, associate array is required");
		}

		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter, update, php::array(nullptr),
		};
		ctx->req.data = ctx;
		if(params.length() > 2) {
			if(params[2].is_true()) {
				ctx->doc3 = php::array(1);	
				ctx->doc3.at("upsert",6) = php::BOOL_TRUE;
			}else if(params[2].is_array()) {
				ctx->doc3 = static_cast<php::array&>(params[2]);
			}
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::update_many_wk, default_cb);
		return flame::async(this);
	}
	php::value collection::find_one(php::parameters& params) {
		php::array& filter = static_cast<php::array&>(params[0]), option;
		if(!filter.is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter, php::array(nullptr)
		};
		ctx->req.data = ctx;
		if(params.length() > 1) {
			php::array& opts = static_cast<php::array&>(params[1]);
			if(opts.is_array()) {
				ctx->doc3 = opts;
			}
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::find_one_wk, find_one_cb);
		return flame::async(this);
	}
	void collection::find_one_cb(uv_work_t* w, int status) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		if(ctx->doc1.is_pointer()) {
			const bson_t* doc = ctx->doc1.ptr<const bson_t>();
			php::array rv(0);
			fill_with(rv, doc);
			ctx->rv = std::move(rv);
			// doc 关联的指针需要销毁
			ctx->self->worker_->queue_work(&ctx->req, collection_implement::find_one_af, default_cb);
		}else{
			ctx->co->next();
			delete ctx;
		}
	}
	php::value collection::find_many(php::parameters& params) {
		php::array& filter = static_cast<php::array&>(params[0]);
		if(!filter.is_array()) {
			throw php::exception("illegal selector, associate array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, filter, php::array(nullptr)
		};
		ctx->req.data = ctx;
		if(params.length() > 1) {
			php::array& opts = static_cast<php::array&>(params[1]);
			if(opts.is_array()) {
				ctx->doc3 = opts;
			}
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::find_many_wk, cursor_cb);
		return flame::async(this);
	}
	void collection::cursor_cb(uv_work_t* w, int status) {
		collection_request_t* ctx = reinterpret_cast<collection_request_t*>(w->data);
		if(ctx->doc1.is_pointer()) {
			mongoc_cursor_t* cs = ctx->doc1.ptr<mongoc_cursor_t>();
			php::object obj = php::object::create<cursor>();
			cursor*     cpp = obj.native<cursor>();
			cpp->init(ctx->self->worker_, ctx->self->cpp_, cs);
			ctx->rv = std::move(obj);
		}
		ctx->co->next(ctx->rv);
		delete ctx;
	}
	php::value collection::aggregate(php::parameters& params) {
		php::array& pipe = static_cast<php::array&>(params[0]);
		if(!pipe.is_array()) {
			throw php::exception("illegal pipeline, index array is required");
		}
		collection_request_t* ctx = new collection_request_t {
			coroutine::current, impl, nullptr, pipe, php::array(nullptr)
		};
		ctx->req.data = ctx;
		if(params.length() > 1) {
			php::array& opts = static_cast<php::array&>(params[1]);
			if(opts.is_array()) {
				ctx->doc3 = opts;
			}
		}
		impl->worker_->queue_work(&ctx->req, collection_implement::aggregate_wk, cursor_cb);
		return flame::async(this);
	}
}
}
}
