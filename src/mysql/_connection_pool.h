#pragma once

namespace flame {
namespace mysql {
	class _connection_pool: public _connection_base, public std::enable_shared_from_this<_connection_pool> {
	public:
		// 以下函数应在主线程调用
		_connection_pool(std::shared_ptr<php::url> url, std::size_t max = 4);
		~_connection_pool();
		_connection_pool& exec(std::shared_ptr<coroutine> co, std::function<void (std::shared_ptr<coroutine> co, std::shared_ptr<MYSQL> c)> wk);
		// 以下函数应在工作线程调用
		void acquire(std::function<void (std::shared_ptr<MYSQL> c)> cb);
		void release(MYSQL* c);
	private:
		std::shared_ptr<php::url> url_;
		std::uint16_t      max_;
		std::list<MYSQL*> conn_;
		std::uint16_t     size_;
		std::list<std::function<void (std::shared_ptr<MYSQL>)>> wait_;
	};
}
}