#include "../controller.h"
#include "../coroutine.h"
#include "_connection_base.h"
#include "_connection_pool.h"

namespace flame {
namespace mysql {
	_connection_pool::_connection_pool(std::shared_ptr<php::url> url, std::size_t max)
	: url_(url)
	, max_(max)
	, size_(0) {

	}
	_connection_pool::~_connection_pool() {
		while(!conn_.empty()) {
			mysql_close(conn_.front());
			conn_.pop_front();
		}
	}
	// 以下函数应在主线程调用
	_connection_pool& _connection_pool::exec(std::shared_ptr<coroutine> co, std::function<void (std::shared_ptr<coroutine> co, std::shared_ptr<MYSQL> c)> wk) {
		auto ptr = this->shared_from_this();
		boost::asio::post(controller_->context_ex,
			// 获取一个连接后执行实际的工作(连接获取工作须在工作线程进行)
			std::bind(&_connection_pool::acquire, ptr, [co, wk, ptr] (std::shared_ptr<MYSQL> c) {
				wk(co, c);
			}));
		return *this;
	}
	// 以下函数应在工作线程调用
	void _connection_pool::acquire(std::function<void (std::shared_ptr<MYSQL> c)> cb) {
		wait_.push_back(std::move(cb));
		while(!conn_.empty()) {
			if(mysql_ping(conn_.front()) == 0) { // 找到了一个可用连接
				release(conn_.front());
			}else{ // 已丢失的连接抛弃
				conn_.pop_front();
				--size_;
			}
		}
		if(size_ >= max_) return; // 已建立了足够多的连接, 需要等待

		MYSQL* c = mysql_init(nullptr);
		if(!mysql_real_connect(c, url_->host, url_->user, url_->pass, url_->path + 1, url_->port, nullptr, 0)) {
			throw std::runtime_error("cannot connect to mysql server");
		}
		++size_; // 当前还存在的连接数量
		release(c);
	}
	void _connection_pool::release(MYSQL* c) {
		if(wait_.empty()) { // 无等待分配的请求
			conn_.push_back(c);
		}else{ // 立刻分配使用
			std::function<void (std::shared_ptr<MYSQL> c)> cb = wait_.front();
			wait_.pop_front();
			std::shared_ptr<MYSQL> p(c, std::bind(&_connection_pool::release, this->shared_from_this(), std::placeholders::_1));
			cb(p);
		}
	}
}
}