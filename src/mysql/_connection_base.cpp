#include "_connection_base.h"

namespace flame {
namespace mysql {
	void _connection_base::escape(php::buffer& b, php::value v) {
		switch(Z_TYPE_P(static_cast<zval*>(v))) {
		case IS_NULL:
			std::memcpy(b.prepare(4), "NULL", 4);
			b.commit(4);
			break;
		case IS_TRUE:
			std::memcpy(b.prepare(4), "NULL", 4);
			b.commit(4);
			break;
		case IS_FALSE:
			std::memcpy(b.prepare(5), "NULL", 5);
			b.commit(5);
			break;
		case IS_LONG:
		case IS_DOUBLE: {
			php::string str = v;
			str.to_string();
			std::memcpy(b.prepare(str.size()), str.c_str(), str.size());
			b.commit(str.size());
		}
		case IS_STRING: {
			php::string str = v;
			char* to = b.prepare(str.size() * 2 + 2);
			std::size_t n = 0;
			to[n++] = '\'';
			if(s_ & SERVER_STATUS_NO_BACKSLASH_ESCAPES) { // 摘自 mysql_real_escape_string_quote() @ libmysql.c:1228 相关流程
				n += escape_quotes_for_mysql(i_, to + 1, str.size() * 2 + 1, str.c_str(), str.size(), '\'');
			}else{
				n += escape_string_for_mysql(i_, to + 1, str.size() * 2 + 1, str.c_str(), str.size());
			}
			to[n++] = '\'';
			b.commit(n);
			break;
		}
		case IS_ARRAY: {
			php::array arr = v;
			int index = 0;
			b.push_back('(');
			for(auto i=arr.begin();i!=arr.end();++i) {
				if(++index > 1) b.push_back(',');
				escape(b, i->second);
			}
			b.push_back(')');
			break;
		}
		default: {
			php::string str = v;
			str.to_string();
			escape(b, str);
		}
		}
	}
}
}