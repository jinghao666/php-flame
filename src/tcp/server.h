#pragma once

namespace flame {
namespace tcp {
	class server: public php::class_base {
	public:
		static void declare(php::extension_entry& ext);
		php::value __construct(php::parameters& params);
		php::value run(php::parameters& params);
	private:
		tcp::endpoint addr_;
		php::callable cb_;

		friend class acceptor;
	};
}
}