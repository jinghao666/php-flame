#include "../controller.h"
#include "../time/time.h"
#include "logger.h"
#include "log.h"

namespace flame {
namespace log {
	php::value logger_ref;
	logger*    logger_ = nullptr;

	static void write_before_exit(const std::exception& ex) {
		boost::format fmt("(FAIL) Uncaught C++ Exception: %s");
		fmt % ex.what();
		// C++ 代码异常需要额外进行输出
		std::clog << "[" << time::datetime() << "] " << fmt.str() << std::endl;
		// 额外记录到日志文件
		if(logger_->to_file()) {
			logger_->write(fmt.str());
		}
	}
	static void write_before_exit(const php::exception& ex) {
		boost::format fmt("(FAIL) Uncaught PHP Exception: %s");
		// PHP 异常可以获得更多的信息
		php::object obj(ex);
		php::string str = obj.call("__tostring");
		fmt % str;
		std::clog << "[" << time::datetime() << "] " << fmt.str() << std::endl;
		// 额外记录到日志文件
		if(logger_->to_file()) {
			logger_->write(fmt.str());
		}
	}

	void declare(php::extension_entry& ext) {
		controller_->on_init([] (const php::array& opts) {
			php::string file;
			if(opts.exists("logger")) {
				file = opts.get("logger");
			}
			logger_ref = php::object(php::class_entry<logger>::entry(), {file});
			logger_    = static_cast<logger*>(php::native(logger_ref));
		})->on_stop([] (std::exception_ptr ex) {
			if(ex) {
				try{
					std::rethrow_exception(ex);
				}catch(const php::exception& ex) {
					write_before_exit(ex);
				}catch(const std::exception& ex) {
					write_before_exit(ex);
				}
			}
			// 一定要提前销毁
			logger_ref = nullptr;
			logger_ = nullptr;
		});
		ext
			.function<rotate>("flame\\log\\rotate")
			.function<write>("flame\\log\\write")
			.function<info>("flame\\log\\info")
			.function<warn>("flame\\log\\warn")
			.function<fail>("flame\\log\\fail");

		logger::declare(ext);
	}
	static void init_guard() {
		if(!logger_) {
			throw php::exception(zend_ce_parse_error, "flame not initialized");
		}
	}
	php::value rotate(php::parameters& params) {
		init_guard();
		return logger_->rotate(params);
	}
	php::value write(php::parameters& params) {
		init_guard();
		return logger_->write(params);
	}
	php::value info(php::parameters& params) {
		init_guard();
		return logger_->info(params);
	}
	php::value warn(php::parameters& params) {
		init_guard();
		return logger_->warn(params);
	}
	php::value fail(php::parameters& params) {
		init_guard();
		return logger_->fail(params);
	}
	
}
}