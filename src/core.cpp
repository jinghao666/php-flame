#include "controller.h"
#include "coroutine.h"
#include "core.h"

namespace flame {
	//
	void declare(php::extension_entry& ext) {
		controller_.reset(new controller());
		ext
			.function<init>("flame\\init", {
				{"title", php::TYPE::STRING},
				{"options", php::TYPE::ARRAY, false, true} // 可选参数
			})
			.function<go>("flame\\go", {
				{"coroutine_fn", php::TYPE::CALLABLE}
			})
			.function<trigger_error>("flame\\trigger_error")
			.function<run>("flame\\run");
	}
	//
	php::value init(php::parameters& params) {
		php::string name = params[0];
		php::array options; // undefined
		if(params.size() > 1) {
			options = params[1];
		}else{
			options = php::array(0);
		}
		// 保持在函数最后执行
		controller_->init(name, options);
		return nullptr;
	}
	php::value go(php::parameters& params) {
		if(!controller_) {
			throw php::exception(zend_ce_parse_error, "flame not initialized");
		}
		// 主控进程时, 不产生实际功能
		if(controller_->type != controller::MASTER) {
			php::callable fn = params[0];
			std::make_shared<coroutine>()->start(fn);
		}
		return nullptr;
	}
	php::value trigger_error(php::parameters& params) {
		std::shared_ptr<coroutine> co = coroutine::current;
		boost::asio::post(context, [co] () {
			co->fail("an Asynchronous Exception");
		});
		return coroutine::async();
	}
	//
	php::value run(php::parameters& params) {
		if(!controller_) {
			throw php::exception(zend_ce_parse_error, "flame not initialized");
		}
		controller_->run();
		return nullptr;
	}
}
