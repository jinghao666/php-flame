#include "../controller.h"
#include "../coroutine.h"
#include "_connection_base.h"
#include "_connection_tx.h"

namespace flame {
namespace mysql {
	_connection_tx::_connection_tx(std::shared_ptr<MYSQL> c)
	: c_(c) {

	}
	// 以下函数应在主线程调用
	_connection_tx& _connection_tx::exec(std::shared_ptr<coroutine> co, std::function<void (std::shared_ptr<coroutine> co, std::shared_ptr<MYSQL> c)> wk) {
		auto ptr = this->shared_from_this();
		boost::asio::post(controller_->context_ex, [co, wk, ptr] () {
			wk(co, ptr->c_); // 访问当前以持有的连接
		});
		return *this;
	}
}
}