#pragma once

namespace flame {
namespace mysql {
	class _connection_tx: public _connection_base, public std::enable_shared_from_this<_connection_tx> {
	public:
		// 以下函数应在工作线程调用
		_connection_tx(std::shared_ptr<MYSQL> c);
		// 以下函数应在主线程调用
		_connection_tx& exec(std::shared_ptr<coroutine> co, std::function<void (std::shared_ptr<coroutine> co, std::shared_ptr<MYSQL> c)> wk);
	private:
		std::shared_ptr<MYSQL> c_;
	};
}
}