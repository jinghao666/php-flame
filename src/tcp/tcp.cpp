#include "../coroutine.h"
#include "tcp.h"
#include "socket.h"
#include "server.h"

namespace flame {
namespace tcp {
	tcp::resolver* resolver_;
	void declare(php::extension_entry& ext) {
		controller_->on_init([] (const php::array& opts) {
			resolver_ = new tcp::resolver(context);
		})->on_stop([] (std::exception_ptr ex) {
			delete resolver_;
			resolver_ = nullptr;
		});
		ext
			.function<connect>("flame\\tcp\\connect", {
				{"address", php::TYPE::STRING},
			});
		socket::declare();
		server::declare();
	}
	php::value connect(php::parameters& params) {
		php::object s(php::class_entry<socket>::entry());
		socket* s_ = static_cast<socket*>(php::native(s));
		php::string str = params[0];
		char *s = str.data(), *p, *e = s + str.size();
		for(p = s; p < e; ++p) {
			if(*p == ':') break; // 分离 地址与端口
		}
		if(*p == ':') throw php::exception(zend_ce_exception, "connect tcp socket failed: address port missing");

		std::shared_ptr<coroutine> co = coroutine::current;
		// DNS 地址解析
		resolver_.async_resolve(boost::string_view(s, p - s), boost::string_view(p + 1, e - p - 1), [str, s, s_, co] (const boost::system::error_code& error, tcp::resolver::results_type edps) {
			if(error) return co->fail(error);
			// 连接
			boost::asio::async_connect(s_->socket_, edps, [s, s_, co] (const boost::system::error_code& error, const tcp::endpoint& edp) {
				if(error) return co->fail(error);
				s.set("local_address", (boost::format("%s:%d") % s_->socket_.local_endpoint().address().to_string() % s_->socket_.local_endpoint().port()).str());
				s.set("remote_address", (boost::format("%s:%d") % s_->socket_.remote_endpoint().address().to_string() % s_->socket_.remote_endpoint().port()).str());
				co->resume(std::move(s));
			});
		});
		return coroutine::async();
	}
}
}