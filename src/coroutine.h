#pragma once

namespace flame {
	extern boost::asio::io_context context;
	extern zend_class_entry* zend_ce_async_error;

	class coroutine: public std::enable_shared_from_this<coroutine> {
	public:
		// 当前协程
		static std::shared_ptr<coroutine> current;
		static php::value async();
		static php::value exception(const php::string& msg, int code = 0);
		coroutine();
		~coroutine();
		// 
		std::shared_ptr<coroutine> stack(const php::callable& fn, const php::object& ref);
		std::shared_ptr<coroutine> stack(const php::callable& fn);
		// 启动协程
		void start(const php::callable& fn);
		void start(const php::callable& fn, std::vector<php::value> argv);
		// 恢复协程上下文并继续
		void resume();
		//
		void resume(php::value rv);
		//
		void resume(std::vector<php::value> rv);
		void fail(const php::string& msg, int code = 0);
		void fail(const boost::system::error_code& error);
	private:
		// 协程堆栈 (嵌套层级 Generator 调用)
		std::stack<std::pair<php::value, php::object>>  st_;
		// 运行结果或启动参数
		std::vector<php::value> rv_;
		int status_;
		boost::asio::executor_work_guard<boost::asio::io_context::executor_type> wk_;
		
		// 协程运行过程
		void start_ex();
		void run_ex();
		void run();
		void tune_ex(php::object& g, php::exception& ex);

		friend class controller;
	};
}