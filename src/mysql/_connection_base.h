#pragma once

namespace flame {
	class coroutine;
namespace mysql {
	class _connection_base {
	public:
		// 以下函数应在主线程调用
		_connection_base()
		:i_(nullptr) {};
		virtual ~_connection_base() {};
		virtual _connection_base& exec(std::function<MYSQL_RES* (std::shared_ptr<MYSQL> c)> wk,
			std::function<void (std::shared_ptr<MYSQL> c, MYSQL_RES* r)> fn) = 0;
		void escape(php::buffer& b, php::value v); // 方便同步使用
	protected:
		CHARSET_INFO* i_; // 为 escape 准备, 方便同步使用
		unsigned int  s_;
	};
}
}