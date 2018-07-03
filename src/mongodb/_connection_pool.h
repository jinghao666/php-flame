#pragma once

namespace flame {
namespace mongodb {
	class _connection_pool: public std::enable_shared_from_this<_connection_pool> {
	public:
		// 以下函数应在主线程调用
		_connection_pool(const php::string& url);
		~_connection_pool();
		_connection_pool& exec(std::function<std::shared_ptr<bson_t> (std::shared_ptr<mongoc_client_t> c)> wk,
			std::function<void (std::shared_ptr<mongoc_client_t> c, std::shared_ptr<bson_t> d)> fn);
	private:

		mongoc_client_pool_t* pool_;
	};
}
}