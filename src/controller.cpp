#include "time/time.h"
#include "controller.h"
#include "coroutine.h"
#include "execinfo.h"

namespace flame {
	std::unique_ptr<controller> controller_;
	controller::controller()
	: type(UNKNOWN)
	, environ(boost::this_process::environment())
	, autorestart_(false) {
		if(environ.count("FLAME_PROCESS_WORKER") == 0) {
			type = MASTER;
		}else{
			type = WORKER;
		}
	}
	void controller::init(const std::string& title, const php::array& opts) {
		environ["FLAME_PROCESS_TITLE"] = title;
		if(type == MASTER) {
			php::callable("cli_set_process_title").call({ title + " (flame/m)" });
			int count = 1;
			if(opts.exists("worker")) {
				count = std::max(opts.get("worker").to_integer(), std::int64_t(1));
			}
			worker_.resize(count);
			for(int i=0; i<worker_.size(); ++i) {
				spawn(i);
			}
		}else{
			php::callable("cli_set_process_title").call({ title + " (flame/" + environ["FLAME_PROCESS_WORKER"].to_string() + ")" });
		}
		php::value debug = opts.get("debug");
		if(!debug.typeof(php::TYPE::UNDEFINED) && debug.empty()) {
			autorestart_ = true;
		}
		for(auto fn: init_) {
			fn(opts);
		}
	}
	controller* controller::on_init(std::function<void (const php::array& opts)> fn) {
		init_.push_back(fn);
		return this;
	}
	controller* controller::on_stop(std::function<void (std::exception_ptr ex)> fn) {
		stop_.push_back(fn);
		return this;
	}
	void controller::run() {
		switch(type) {
		case MASTER:
			master_run();
			break;
		case WORKER:
			worker_run();
			break;
		default:
			assert(0 && "未知进程类型");
		}
	}
	void controller::spawn(int i) {
		std::string executable = php::constant("PHP_BINARY");
		std::string script = php::server("SCRIPT_FILENAME");
		boost::process::environment env = environ;
		env["FLAME_PROCESS_WORKER"] = std::to_string(i+1);
		worker_[i] = boost::process::child(executable, script, env, g_, context,
			boost::process::std_out > stdout,
			boost::process::std_err > stdout,
			boost::process::on_exit = [this, i] (int exit_code, const std::error_code&) {
			if(exit_code != 0 && autorestart_) {
				std::clog << "[" << time::datetime() << "] (WARN) worker unexpected exit, restart in 3s ..." << std::endl;
				auto tm = std::make_shared<boost::asio::steady_timer>(context, std::chrono::seconds(3));
				tm->async_wait([this, tm, i] (const boost::system::error_code& error) {
					if(!error) spawn(i);
				});
			}else{
				for(int i=0;i<worker_.size();++i) {
					if(worker_[i].running()) return;
				}
				context.stop();
			}
		});
	}
	void controller::master_run() {
		// 主进程负责进程监控, 转发信号
		boost::asio::signal_set s(context, SIGINT, SIGTERM);
		s.async_wait([this] (const boost::system::error_code& error, int sig) {
			for(int i=0; i<worker_.size(); ++i) {
				::kill(worker_[i].id(), sig);
			}
			context.stop();
		});
		context.run();
		for(auto fn: stop_) {
			fn(nullptr);
		}
		// 这里有两种情况:
		// 1. 所有子进程自主退出;
		// 2. 子进程还未退出, 一段时间后强制结束;
		std::error_code error;
		if(!g_.wait_for(std::chrono::seconds(10), error)) {
			g_.terminate(); // 10s 后还未结束, 强制杀死
		}
	}
	void controller::worker_run() {
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> idle(context_ex.get_executor());
		// 辅助工作线程
		std::thread ex_thread([this] () {
			context_ex.run();
			::sleep(1);
		});
		std::exception_ptr ex;
		// 工作进程负责执行
		try{
			context.run();
		}catch(...) {
			ex = std::current_exception();
		}
		for(auto fn: stop_) {
			fn(ex);
		}
		context_ex.stop();
		ex_thread.join();
		if(ex) exit(-1); // !!! 发生异常退出, 防止 PHP 引擎将还存活的对象内存提前释放
	}
}