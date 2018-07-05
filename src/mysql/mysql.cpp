#include "../controller.h"
#include "../coroutine.h"
#include "mysql.h"
#include "_connection_base.h"
#include "_connection_pool.h"
#include "transaction.h"
#include "client.h"
#include "result.h"

namespace flame {
namespace mysql {
	void declare(php::extension_entry& ext) {
		controller_->on_init([] (const php::array& opts) {
			mysql_library_init(0, nullptr, nullptr);
		})->on_stop([] (std::exception_ptr ex) {
			mysql_library_end();
		});
		ext
			.function<connect>("flame\\mysql\\connect");
		transaction::declare(ext);
		client::declare(ext);
		result::declare(ext);
	}
	php::value connect(php::parameters& params) {
		php::object cli(php::class_entry<client>::entry());
		client* cli_ = static_cast<client*>(php::native(cli));

		cli_->c_.reset(new _connection_pool(php::parse_url(params[0])));
		std::shared_ptr<coroutine> co = coroutine::current;
		cli_->c_->exec([] (std::shared_ptr<MYSQL> c, int& error) -> std::shared_ptr<MYSQL_RES> {
			return nullptr;
		}, [co, cli] (std::shared_ptr<MYSQL> c, std::shared_ptr<MYSQL_RES> rs, int error) {
			co->resume(cli);
		});
		return coroutine::async();
	}
	// 相等 {==} / {=}
	static void where_eq(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append(" IS NULL", 8);
		}else{
			php::string str = cond;
			str.to_string();
			buf.push_back('=');
			cc->escape(buf, str);
		}
	}
	// 不等 {!=}
	static void where_ne(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append(" IS NOT NULL", 12);
		}else{
			php::string str = cond;
			str.to_string();
			buf.append("!=");
			cc->escape(buf, str);
		}
	}
	// 大于 {>}
	static void where_gt(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append(">0", 2);
		}else{
			php::string str = cond;
			str.to_string();
			buf.push_back('>');
			cc->escape(buf, str);
		}
	}
	// 小于 {<}
	static void where_lt(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append("<0", 2);
		}else{
			php::string str = cond;
			str.to_string();
			buf.push_back('<');
			cc->escape(buf, str);
		}
	}
	// 大于等于 {>=}
	static void where_gte(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append(">=0", 3);
		}else{
			php::string str = cond;
			str.to_string();
			buf.append(">=");
			cc->escape(buf, str);
		}
	}
	// 小于等于 {<=}
	static void where_lte(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& cond) {
		if(cond.typeof(php::TYPE::NULLABLE)) {
			buf.append("<=0", 3);
		}else{
			php::string str = cond;
			str.to_string();
			buf.append("<=");
			cc->escape(buf, str);
		}
	}
 	// 某个 IN 无对应符号
	static void where_in(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::array& cond) {
		assert(cond.typeof(php::TYPE::ARRAY) && "目标格式错误");

		buf.append(" IN (", 5);
		for(auto i=cond.begin(); i!=cond.end(); ++i) {
			if(static_cast<int>(i->first) > 0) {
				buf.push_back(',');
			}
			php::string v = i->second;
			v.to_string();
			cc->escape(buf, v);
		}
		buf.push_back(')');
	}
	static void where_ex(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::array& cond, const php::string& field, const php::string& separator) {
		if(cond.size() > 1) buf.append(" (", 2);
		
		int j = -1;
		for(auto i=cond.begin(); i!=cond.end(); ++i) {
			if(++j > 0) buf.append(separator);

			if(i->first.typeof(php::TYPE::INTEGER)) {
				if(i->second.typeof(php::TYPE::ARRAY)) {
					where_ex(cc, buf, i->second, i->first, " && ");
				}else{
					php::string str = i->second;
					str.to_string();
					buf.append(str);
				}
			}else {
				php::string key = i->first;
				if(key.c_str()[0] == '{') { // OPERATOR (php::TYPE::STRING)
					if(key.size() == 4 && strncasecmp(key.c_str(), "{OR}", 4) == 0) {
						assert(i->second.typeof(php::TYPE::ARRAY));
						where_ex(cc, buf, i->second, php::string(nullptr), " || ");
					}else if(key.size() == 5 && strncasecmp(key.c_str(), "{AND}", 5) == 0) {
						assert(i->second.typeof(php::TYPE::ARRAY));
						where_ex(cc, buf, i->second, php::string(nullptr), " && ");
					}else if(key.size() == 3 && strncasecmp(key.c_str(), "{=}", 3) == 0 || key.size() == 4 && strncasecmp(key.c_str(), "{==}", 4) == 0) {
						cc->escape(buf, field, '`');
						where_eq(cc, buf, i->second);
					}else if(key.size() == 4 && strncasecmp(key.c_str(), "{!=}", 4) == 0) {
						cc->escape(buf, field, '`');
						where_ne(cc, buf, i->second);
					}else if(key.size() == 3 && strncasecmp(key.c_str(), "{>}", 3) == 0) {
						cc->escape(buf, field, '`');
						where_gt(cc, buf, i->second);
					}else if(key.size() == 3 && strncasecmp(key.c_str(), "{<}", 2) == 0) {
						cc->escape(buf, field, '`');
						where_lt(cc, buf, i->second);
					}else if(key.size() == 4 && strncasecmp(key.c_str(), "{>=}", 4) == 0) {
						cc->escape(buf, field, '`');
						where_gte(cc, buf, i->second);
					}else if(key.size() == 4 && strncasecmp(key.c_str(), "{<=}", 4) == 0) {
						cc->escape(buf, field, '`');
						where_lte(cc, buf, i->second);
					}
				}else{ // php::TYPE::STRING
					if(i->second.typeof(php::TYPE::ARRAY)) {
						php::array cond = i->second;
						if(cond.exists(0)) {
							cc->escape(buf, key, '`');
							buf.push_back(' ');
							where_in(cc, buf, i->second);
						}else{
							where_ex(cc, buf, cond, key, " && ");
						}
					}else{
						cc->escape(buf, key, '`');
						where_eq(cc, buf, i->second);
					}
				}
			}
		}
		if(cond.size() > 1) buf.push_back(')');
	}
	void build_where(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& data) {
		buf.append("WHERE", 5);
		if(data.typeof(php::TYPE::STRING)) {
			buf.append(data);
		}else if(data.typeof(php::TYPE::ARRAY)) {
			where_ex(cc, buf, data, php::string(0), " && ");
		}else{
			throw php::exception(zend_ce_type_error, "failed to build where conditions: unsupported type");
		}
	}
	// void build_order(std::shared_ptr<_connection_base> cc, php::buffer& buf, php::value& data) {
	// 	if(data.is_string()) {
	// 		std::memcpy(buf.put(10), " ORDER BY ", 10);
	// 		php::string& str = static_cast<php::string&>(data);
	// 		std::memcpy(buf.put(str.length()), str.c_str(), str.length());
	// 	}else if(data.is_array()) {
	// 		std::memcpy(buf.put(10), " ORDER BY ", 10);
	// 		php::array& sort = static_cast<php::array&>(data);
	// 		int j = -1;
	// 		for(auto i=sort.begin();i!=sort.end();++i) {
	// 			php::string key = i->first.to_string();
	// 			int64_t dir = i->second.to_long();
	// 			if(++j > 0) buf.add(',');
	// 			if(dir > 0) {
	// 				buf.add('`');
	// 				std::memcpy(buf.put(key.length()), key.c_str(), key.length());
	// 				std::memcpy(buf.put(5), "` ASC", 5);
	// 			}else{
	// 				buf.add('`');
	// 				std::memcpy(buf.put(key.length()), key.c_str(), key.length());
	// 				std::memcpy(buf.put(6), "` DESC", 6);
	// 			}
	// 		}
	// 	}else if(data.is_null()) {

	// 	}else{
	// 		throw php::exception("illegl sql order by");
	// 	}
	// }
	// void build_limit(std::shared_ptr<_connection_base> cc, php::buffer& buf, php::value& data) {
	// 	std::memcpy(buf.put(7), " LIMIT ", 7);
	// 	if(data.is_string()) {
	// 		php::string& str = static_cast<php::string&>(data);
	// 		std::memcpy(buf.put(str.length()), str.data(), str.length());
	// 	}else if(data.is_long()) {
	// 		int64_t x = data;
	// 		size_t  n = sprintf(buf.rev(10), "%ld", x);
	// 		buf.adv(n);
	// 	}else if(data.is_array()) {
	// 		php::array& limit = static_cast<php::array&>(data);
	// 		int64_t x = limit[0], y = 0;
	// 		if(limit.length() > 1) {
	// 			y = limit[1];
	// 		}
	// 		if(y == 0) {
	// 			size_t  n = sprintf(buf.rev(10), "%ld", x);
	// 			buf.adv(n);
	// 		}else{
	// 			size_t  n = sprintf(buf.rev(20), "%ld,%ld", x, y);
	// 			buf.adv(n);
	// 		}
	// 	}else{
	// 		throw php::exception("illegal sql limit");
	// 	}
	// }
}
}