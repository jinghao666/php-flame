#include "../dynamic_buffer.h"
#include "../coroutine.h"
#include "_client.h"
#include "_command_base.h"
#include "_command.h"

namespace flame {
namespace redis {
	_client::_client()
	: socket_(context)
	, status_(0) {

	}
	void _client::connect(std::shared_ptr<flame::coroutine> co, std::shared_ptr<php::url> url) {
		tcp::endpoint e (boost::asio::ip::make_address(url->host), url->port ? url->port : 6379);
		auto ptr = this->shared_from_this();
		if(std::strlen(url->path) > 1) {
			co->stack(php::value([this, ptr, co, url] (php::parameters& params) -> php::value {
				php::array arg(1);
				arg[0] = php::string(url->path + 1);
				send(co, new _command("SELECT", arg));
				return coroutine::async();
			}));
		}
		if(url->pass) {
			co->stack(php::value([this, ptr, co, url] (php::parameters& params) -> php::value {
				php::array arg(1);
				arg[0] = php::string(url->pass);
				send(co, new _command("AUTH", arg));
				return coroutine::async();
			}));
		}
		socket_.async_connect(e, [this, ptr, co] (const boost::system::error_code& error) {
			if(error) {
				co->fail(error);
			}else{
				co->resume();
			}
		});
	}
	void _client::send(std::shared_ptr<flame::coroutine> co, _command_base* base) {
		base->co_ = co;
		qs_.push_back(base);
		if(status_ & STATUS_FLAG_SENDING) return;
		coro_s = boost::asio::coroutine();
		send_ex();
	}
	void _client::read() {
		qr_.splice(qr_.end(), qs_, qs_.begin());
		if(status_ & STATUS_FLAG_READING) return;
		coro_r = boost::asio::coroutine();
		read_ex();
	}
	// 若不存在 STATUS_FLAG_WRITING 则开始发送；
	void _client::send_ex(const boost::system::error_code& error, std::size_t n) { BOOST_ASIO_CORO_REENTER(coro_s) {
		status_ |= STATUS_FLAG_SENDING;
		while(!qs_.empty()) {
			// 发送队头元素的所有 BUFFER
			while(qs_.front()->writer() > 0) {
				BOOST_ASIO_CORO_YIELD boost::asio::async_write(socket_, qs_.front()->sbuffer_, std::bind(&_client::send_ex, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
				if(error) {
					qs_.front()->co_->fail(error);
					return;
				}
			}
			// 挪动到接收队列并启动接收过程（若还未启动）
			read();
		}
		status_ &= ~STATUS_FLAG_SENDING;
	} }
	// 若不存在 STATUS_FLAG_READING 则开始接收；
	void _client::read_ex(const boost::system::error_code& error, std::size_t n) { BOOST_ASIO_CORO_REENTER(coro_r) {
		status_ |= STATUS_FLAG_READING;
		while(!qr_.empty()) {
			// 接受队头元素所有数据
			while(qr_.front()->rv_.size() > 1 || qr_.front()->rv_.top().size > 0) {
				BOOST_ASIO_CORO_YIELD boost::asio::async_read_until<tcp::socket, dynamic_buffer>(socket_, dynamic_buffer(rbuffer_), "\r\n", std::bind(&_client::read_ex, this->shared_from_this(), std::placeholders::_1, std::placeholders::_2));
				if(error) {
					qr_.front()->co_->fail(error);
					return;
				}
				// 注意: 实际接受的数据 buffer_.size() 可能会多余 n 指示的长度 (但 reader 每次仅处理一行数据)
				rbuffer_.consume(qr_.front()->reader(rbuffer_.data(), n));
			}
			if(qr_.front()->rt_ == '-') {
				qr_.front()->co_->fail(qr_.front()->rv_.top().value);
			}else{
				qr_.front()->co_->resume(qr_.front()->rv_.top().value);
			}
			delete qr_.front();
			qr_.pop_front();
		}
		status_ &= ~STATUS_FLAG_READING;
	} }
}
}