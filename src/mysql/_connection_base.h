#pragma once

namespace flame {
namespace mysql {
	class _connection_base {
	public:
		// 以下函数应在主线程调用
		_connection_base() {};
		virtual ~_connection_base() {};
		virtual _connection_base& exec(std::shared_ptr<coroutine> co, std::function<void (std::shared_ptr<coroutine> co, std::shared_ptr<MYSQL> c)> wk) = 0;
	};
}
}