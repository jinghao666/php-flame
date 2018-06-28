#pragma once

namespace flame {
	class coroutine;
	class controller {
	public:
		enum process_type {
			UNKNOWN = 0,
			MASTER  = 1,
			WORKER  = 2,
		} type;
		boost::asio::io_context     context_ex;
		boost::process::environment environ;
	private:
		
		std::vector<boost::process::child> worker_;
		boost::process::group                   g_;
		bool                          autorestart_;
		std::list<std::function<void (const php::array& opts)>> init_;
		std::list<std::function<void (std::exception_ptr ex)>>  stop_;
	public:
		controller();
		controller(const controller& c) = delete;
		void init(const std::string& title, const php::array& opts);
		void run();
		controller* on_init(std::function<void (const php::array& opts)> fn);
		controller* on_stop(std::function<void (std::exception_ptr ex)> fn);
	private:
		void spawn(int i);
		void master_run();
		void worker_run();
	};
	extern std::unique_ptr<controller> controller_;
}