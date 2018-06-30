#pragma once

namespace flame {
	class coroutine;
namespace udp {
	class socket: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		socket();
		php::value receive(php::parameters& param);
		php::value receive_from(php::parameters& params);
		php::value send(php::parameters& params);
		php::value send_to(php::parameters& params);
		php::value close(php::parameters& params);
		void write_ex();
	private:
		udp::socket socket_;
		php::buffer buffer_;
		std::list< std::pair<std::shared_ptr<coroutine>, php::string> > q_;
	};
}
}