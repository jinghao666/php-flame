#pragma once

namespace flame {
namespace time {
	void declare(php::extension_entry& ext);
	php::value now(php::parameters& params);
	php::value sleep(php::parameters& params);
	php::value after(php::parameters& params);
	php::value tick(php::parameters& params);
	std::chrono::time_point<std::chrono::system_clock> now();
	const char* datetime();
	const char* datetime(std::chrono::time_point<std::chrono::system_clock> t);
}
}