#include "deps.h"
#include "../flame.h"
#include "../coroutine.h"
#include "ticker.h"

namespace flame {
namespace time {
	php::value ticker::__construct(php::parameters& params) {
		if(!params[0].is_long()) {
			throw php::exception("tick interval of type integer is required");
		}
		prop("interval", 8) = static_cast<int>(params[0]);
		if(params.length() > 1) {
			prop("repeat", 6) = params[1].is_true() ? php::BOOL_TRUE : php::BOOL_FALSE;
		}else{
			prop("repeat", 6) = php::BOOL_TRUE;
		}
		tm_ = (uv_timer_t*)malloc(sizeof(uv_timer_t));
		uv_timer_init(flame::loop, tm_);
		tm_->data = this;
		return nullptr;
	}
	php::value ticker::__destruct(php::parameters& params) {
		uv_timer_stop(tm_);
		uv_close((uv_handle_t*)tm_, flame::free_handle_cb);
		return nullptr;
	}
	void ticker::tick_cb(uv_timer_t* handle) {
		ticker* self = static_cast<ticker*>(handle->data);
		coroutine::create(self->cb_, {php::value(self)})->after(after_cb, self)->start();
	}
	void ticker::after_cb(void* data) {
		ticker* self = static_cast<ticker*>(data);
		if(!self->prop("repeat").is_true()) {
			self->ref_ = nullptr; // 非重复定时器立即清理引用
		}
	}
	php::value ticker::start(php::parameters& params) {
		cb_ = params[0];

		int iv = prop("interval", 8);
		if(prop("repeat", 6).is_true()) {
			uv_timer_start(tm_, tick_cb, iv, iv);
		}else{
			uv_timer_start(tm_, tick_cb, iv, 0);
		}
		// 异步引用
		ref_ = php::object(this);
		return nullptr;
	}
	php::value ticker::stop(php::parameters& params) {
		uv_timer_stop(tm_);
		ref_ = nullptr;
		return nullptr;
	}
	php::value after(php::parameters& params) {
		php::object tick = php::object::create<ticker>();
		tick.call("__construct", 11, {params[0], php::BOOL_FALSE});
		php::callable cb = params[1];
		tick.call("start", 5, {cb});
		return std::move(tick);
	}
	php::value tick(php::parameters& params) {
		php::object tick = php::object::create<ticker>();
		tick.call("__construct", 11, {params[0], php::BOOL_TRUE});
		php::callable cb = params[1];
		tick.call("start", 5, {cb});
		return std::move(tick);
	}
}
}
