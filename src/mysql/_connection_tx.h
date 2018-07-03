#pragma once

namespace flame {
namespace mysql {
	class _connection_tx: public _connection_base, public std::enable_shared_from_this<_connection_tx> {
	public:
		// 以下函数应在工作线程调用
		_connection_tx(std::shared_ptr<MYSQL> c);
		// 以下函数应在主线程调用
		virtual _connection_tx& exec(std::function<MYSQL_RES* (std::shared_ptr<MYSQL> c)> wk,
			std::function<void (std::shared_ptr<MYSQL> c, MYSQL_RES* r)> fn) override;
	private:
		std::shared_ptr<MYSQL> c_;
	};
}
}