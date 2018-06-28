#include "../coroutine.h"
#include "time.h"
#include "timer.h"

namespace flame {
namespace time {
	static std::chrono::time_point<std::chrono::system_clock> time_system;
	static std::chrono::time_point<std::chrono::steady_clock> time_steady;
	void declare(php::extension_entry& ext) {
		// 记录时间基础值
		time_steady = std::chrono::steady_clock::now();
		time_system = std::chrono::system_clock::now();

		ext
			.function<now>("flame\\time\\now")
			.function<sleep>("flame\\time\\sleep", {
				{"duration", php::TYPE::INTEGER}
			})
			.function<tick>("flame\\time\\tick", {
				{"duration", php::TYPE::INTEGER},
				{"callback", php::TYPE::CALLABLE},
			}).function<after>("flame\\time\\after", {
				{"duration", php::TYPE::INTEGER},
				{"callback", php::TYPE::CALLABLE},
			});

		timer::declare(ext);
	}
	std::chrono::time_point<std::chrono::system_clock> now() {
		std::chrono::milliseconds diff = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - time_steady);
		return time_system + diff;
	}
	const char* datetime() {
		return datetime(now());
	}
	const char* datetime(std::chrono::time_point<std::chrono::system_clock> n) {
		static char cache[24];
		std::time_t t = std::chrono::system_clock::to_time_t(n);
		struct tm*  m = std::localtime(&t);
		sprintf(cache, "%04d-%02d-%02d %02d:%02d:%02d", 
			1900 + m->tm_year,
			1 + m->tm_mon,
			m->tm_mday,
			m->tm_hour,
			m->tm_min,
			m->tm_sec);
		return cache;
	}
	php::value now(php::parameters& params) {
		return std::chrono::duration_cast<std::chrono::milliseconds>(now().time_since_epoch()).count();
	}
	php::value sleep(php::parameters& params) {
		// 绕过类型校验
		std::int64_t d = params[0].to_integer();
		auto tm = std::make_shared<boost::asio::steady_timer>(context, std::chrono::milliseconds(d));
		auto co = coroutine::current;
		tm->async_wait([co, tm] (const boost::system::error_code& error) {
			assert(!error);
			co->resume();
		});
		return coroutine::async();
	}
	php::value tick(php::parameters& params) {
		php::value interval = params[0], cb = params[1];
		std::make_shared<coroutine>()->start(php::value([interval, cb] (php::parameters& params) -> php::value {
			php::object timer(php::class_entry<timer>::entry(), {interval, cb});
			return timer.call("run");
		}));
		return nullptr;
	}
	php::value after(php::parameters& params) {
		php::value interval = params[0], cb = params[1];
		std::make_shared<coroutine>()->start(php::value([interval, cb] (php::parameters& params) -> php::value {
			php::object timer(php::class_entry<timer>::entry(), {interval, cb});
			return timer.call("run", {true});
		}));
		return nullptr;
	}
}
}