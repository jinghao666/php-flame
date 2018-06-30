#include "../coroutine.h"
#include "socket.h"

namespace flame {
namespace udp {
	void socket::declare(php::extension_entry& ext) {
		php::class_entry<socket> class_socket("flame\\udp\\socket");
		class_socket
			.property({"local_address", ""})
			.property({"remote_address", ""})
			.method<&socket::__construct>("__construct", {
				{"bind", php::TYPE::STRING, false, true},
			})
			.method<&socket::receive>("receive")
			.method<&socket::receive>("receive_from", {
				{"from", php::TYPE::STRING, true},
			})
			.method<&socket::write>("send", {
				{"data", php::TYPE::STRING}
			})
			.method<&socket::write>("send_to", {
				{"data", php::TYPE::STRING},
				{"to", php::TYPE::STRING},
			})
			.method<&socket::close>("close");
		ext.add(std::move(class_socket));
	}
	socket::socket()
	: socket_(context) {

	}
	typedef boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_REUSEPORT> reuse_port;
	php::value socket::__construct(php::parameters& params) {
		if(params.size() > 0) {
			php::string str = params[0];
			char *s = str.data(), *p, *e = s + str.size();
			for(p = s; p < e; ++p) {
				// 分离 地址与端口
				if(*p == ':') break;
			}
			if(*p != ':') throw php::exception(zend_ce_exception, "udp socket __construct failed: address port missing");
			udp::endpoint addr(boost::asio::ip::make_address(s, p - s), std::atoi(p+1));

			boost::asio::socket_base::reuse_address opt1(true);
			socket_.set_option(opt1);
			reuse_port opt2(true);
			socket_.set_option(opt2);

			socket_.bind(addr);
		}
		return nullptr;
	}
	php::value socket::receive(php::parameters& param) {
		php::object ref(this);
		std::shared_ptr<coroutine> co = coroutine::current;
		socket_.async_receive(boost::asio::buffer(buffer_.prepare(64 * 1024), 64 * 1024), [this, co, ref] (const boost::system::error_code& error, std::size_t n) {
			if(error) return co->fail(error);
			// buffer_.commit(n);
			co->resume(php::string(buffer_.data(), n)); // 复制 (一般实际的接受量都很小, 避免重复申请 64K 内存)
		});
		return coroutine::async();
	}
	php::value socket::receive_from(php::parameters& params) {
		php::string from = params[0];
		php::object ref(this);
		std::shared_ptr<coroutine> co = coroutine::current;
		std::shared_ptr<udp::endpoint> edp = std::make_shared<udp::endpoint>();
		socket_.async_receive(boost::asio::buffer(buffer_.prepare(64 * 1024), 64 * 1024), *edp, [this, co, edp, from, ref] (const boost::system::error_code& error, std::size_t n) {
			if(error) return co->fail(error);
			// buffer_.commit(n);
			from = (boost::format("%s:%d") % edp.address().to_string() % edp.port()).str();
			co->resume(php::string(buffer_.data(), n)); // 复制 (一般实际的接受量都很小, 避免重复申请 64K 内存)
		});
		return coroutine::async();
	}
	php::value socket::send(php::parameters& params) {
		php::string data = params[0];
		php::object ref(this);
		std::shared_ptr<coroutine> co = coroutine::current;
		socket_.async_send(boost::asio::buffer(data.c_str(), data.size()), [this, co, data, ref] (const boost::system::error_code& error, std::size_t n) {
			if(error) return co->fail(error);
			co->resume();
		});
		return coroutine::async();
	}
	php::value socket::send_to(php::parameters& params) {
		php::string str = params[0];
		char *s = str.data(), *p, *e = s + str.size();
		for(p = s; p < e; ++p) {
			// 分离 地址与端口
			if(*p == ':') break;
		}
		if(*p != ':') throw php::exception(zend_ce_exception, "udp socket send failed: address port missing");
		udp::endpoint addr(boost::asio::ip::make_address(s, p - s), std::atoi(p+1));

		php::string data = params[0];
		php::object ref(this);
		std::shared_ptr<coroutine> co = coroutine::current;
		socket_.async_send_to(boost::asio::buffer(data.c_str(), data.size()), addr, [this, co, data, ref] (const boost::system::error_code& error, std::size_t n) {
			if(error) return co->fail(error);
			co->resume();
		});
		return coroutine::async();
	}
	php::value socket::close(php::parameters& params) {
		socket_.close();
		return nullptr;
	}
}
}
