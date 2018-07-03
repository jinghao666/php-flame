#include "../controller.h"
#include "../coroutine.h"
#include "udp.h"
#include "socket.h"

namespace flame {
namespace udp {
	boost::asio::ip::udp::resolver* resolver_;
	void declare(php::extension_entry& ext) {
		controller_->on_init([] (const php::array& opts) {
			resolver_ = new boost::asio::ip::udp::resolver(context);
		})->on_stop([] (std::exception_ptr ex) {
			delete resolver_;
			resolver_ = nullptr;
		});
		ext
			.function<connect>("flame\\udp\\connect", {
				{"address", php::TYPE::STRING},
			});
		socket::declare(ext);
	}
	php::value connect(php::parameters& params) {
		php::object o(php::class_entry<socket>::entry());
		socket* o_ = static_cast<socket*>(php::native(o));
		php::string str = params[0];
		char *s = str.data(), *p, *e = s + str.size();
		for(p = s; p < e; ++p) {
			if(*p == ':') break; // 分离 地址与端口
		}
		if(*p != ':') throw php::exception(zend_ce_exception, "connect udp socket failed: address port missing");

		std::shared_ptr<coroutine> co = coroutine::current;
		// DNS 地址解析
		resolver_->async_resolve(std::string(s, p - s), std::string(p + 1, e - p - 1), [o, o_, co] (const boost::system::error_code& error, boost::asio::ip::udp::resolver::results_type edps) {
			if(error) return co->fail(error);
			// 连接
			boost::asio::async_connect(o_->socket_, edps, [o, o_, co] (const boost::system::error_code& error, const boost::asio::ip::udp::endpoint& edp) mutable {
				if(error) return co->fail(error);
				o.set("local_address", (boost::format("%s:%d") % o_->socket_.local_endpoint().address().to_string() % o_->socket_.local_endpoint().port()).str());
				o.set("remote_address", (boost::format("%s:%d") % o_->socket_.remote_endpoint().address().to_string() % o_->socket_.remote_endpoint().port()).str());
				co->resume(std::move(o));
			});
		});
		return coroutine::async();
	}
}
}