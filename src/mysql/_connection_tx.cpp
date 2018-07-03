#include "../controller.h"
#include "../coroutine.h"
#include "_connection_base.h"
#include "_connection_tx.h"

namespace flame {
namespace mysql {
	_connection_tx::_connection_tx(std::shared_ptr<MYSQL> c)
	: c_(c) {
		i_ = c->charset;
		s_ = c->server_status;
	}
	// 以下函数应在主线程调用
	_connection_tx& _connection_tx::exec(std::function<MYSQL_RES* (std::shared_ptr<MYSQL> c)> wk,
			std::function<void (std::shared_ptr<MYSQL> c, MYSQL_RES* r)> fn) {
		auto ptr = this->shared_from_this();
		boost::asio::post(controller_->context_ex, [this, ptr, wk, fn] () {
			MYSQL_RES* r = wk(c_);
			boost::asio::post(context, [this, fn, r, ptr] () {
				fn(c_, r);
			});
		}); // 访问当前已持有的连接
		return *this;
	}
}
}