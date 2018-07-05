#pragma once

namespace flame {
namespace mysql {
	class _connection_base;
	void declare(php::extension_entry& ext);
	php::value connect(php::parameters& params);
	void build_where(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& data);
	void build_order(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& data);
	void build_limit(std::shared_ptr<_connection_base> cc, php::buffer& buf, const php::value& data);
}
}