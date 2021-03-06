#include "deps.h"
#include "../../flame.h"
#include "../../coroutine.h"
#include "../../thread_worker.h"
#include "mysql.h"
#include "client_implement.h"
#include "client.h"
#include "result_set.h"

namespace flame {
namespace db {
namespace mysql {
	void client::val_to_buffer(php::value& val, php::buffer& buf) {
		if(val.is_null()) {
			std::memcpy(buf.put(4), "NULL", 4);
		}else if(val.is_true()) {
			std::memcpy(buf.put(4), "TRUE", 4);
		}else if(val.is_false()) {
			std::memcpy(buf.put(5), "FALSE", 5);		
		}else if(val.is_string()) {
			buf.add('\'');
			php::string& str = static_cast<php::string&>(val);
			char*        esp = buf.rev(str.length() * 2);
			size_t       len = mysql_real_escape_string(&impl->mysql_, esp, str.data(), str.length());
			buf.adv(len);
			buf.add('\'');
		}else if(val.is_long()) {
			long   x = val;
			size_t n = sprintf(buf.rev(16), "%ld", x);
			buf.adv(n);
		}else if(val.is_array()) {
			php::array& row = static_cast<php::array&>(val);
			int j = -1;
			for(auto i=row.begin(); i!= row.end(); ++i) {
				if(++j == 0) buf.add('(');
				else buf.add(',');
				val_to_buffer(i->second, buf);
			}
			buf.add(')');
		}else{
			buf.add('\'');
			php::string& str = val.to_string();
			char*        esp = buf.rev(str.length() * 2);
			size_t       len = mysql_real_escape_string(&impl->mysql_, esp, str.data(), str.length());
			buf.adv(len);
			buf.add('\'');
		}
	}
	void client::key_to_buffer(php::array& map, php::buffer& buf) {
		int j = -1;
		for(auto i= map.begin(); i!= map.end(); ++i) {
			php::string key = i->first.to_string();
			if(++j > 0) buf.add(',');
			else buf.add('(');
			buf.add('`');
			std::memcpy(buf.put(key.length()), key.data(), key.length());
			buf.add('`');
		}
		buf.add(')');
	}
	php::value client::__construct(php::parameters& params) {
		impl = new client_implement(this);
		if(params.length() > 0 && params[0].is_array()) {
			php::array& opts = static_cast<php::array&>(params[0]);
			if(opts.at("debug", 5).is_true()) {
				impl->debug_ = true;
			}
			php::value ping = opts.at("ping", 4);
			if(ping.is_long()) {
				ping_interval = static_cast<int>(ping);
				if(ping_interval < 15000) {
					ping_interval = 15 * 1000;
				}
			}else{
				ping_interval = 60 * 1000;
			}
		}else{ // 默认
			ping_interval = 60 * 1000;
		}
		return nullptr;
	}
	php::value client::__destruct(php::parameters& params) {
		impl->destroy();
		return nullptr;
	}
	php::value client::connect(php::parameters& params) {
		// 指定参数时连接指定的服务器
		if(params.length() > 0 && params[0].is_string()) {
			php::string& url = static_cast<php::string&>(params[0]);
			url_ = php::parse_url(url.c_str(), url.length());
			if(!url_ || !url_->scheme || !url_->path || ! url_->host
				|| strncasecmp(url_->scheme, "mysql", 5) != 0 || std::strlen(url_->path) < 1) {
				throw php::exception("failed to parse mysql connection uri", 36);
			}
			if(!url_->port) { // 默认端口
				url_->port = 3306;
			}
		}
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl, nullptr
		};
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, client_implement::connect_wk, connect_cb);
		return flame::async(this);
	}
	void client::connect_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		// 使用 rv 可能带回错误信息
		if(ctx->code != 0) {
			ctx->co->fail(ctx->sql, ctx->code);
		}else{
			ctx->self->start();
			ctx->co->next(ctx->rv);
		}
		delete ctx;
	}
	php::value client::format(php::parameters& params) {
		if(params.length() < 1 || !params[0].is_string()) {
			throw php::exception("format string is required");
		}
		php::string sql = static_cast<php::string&>(params[0]);
		int         idx = 0;
		php::buffer buf;
		for(char* c = sql.data(); c != sql.data() + sql.length(); ++c) {
			if(*c == '?') {
				++idx;
				val_to_buffer(params[idx], buf);
			}else{
				buf.add(*c);
			}
		}
		return std::move(buf);
	}
	void client::query_(const php::string& sql) {
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl, nullptr, sql
		};
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, client_implement::query_wk, default_cb);
	}
	php::value client::query(php::parameters& params) {
		if(params.length() < 1 || !params[0].is_string()) {
			throw php::exception("sql string is required");
		}
		php::string sql;
		if(params.length() > 1) {
			sql = format(params);
		}else{
			sql = static_cast<php::string&>(params[0]);
		}
		query_(sql);
		return flame::async(this);
	}
	php::value client::insert(php::parameters& params) {
		if(params.length() < 2 || !params[0].is_string() || !params[1].is_array()) {
			throw php::exception("table name and insert data array is required");
		}
		php::string& table = static_cast<php::string&>(params[0]);
		php::array&  data  = static_cast<php::array&>(params[1]);

		php::buffer sql;
		std::memcpy(sql.put(13), "INSERT INTO `", 13);
		std::memcpy(sql.put(table.length()), table.c_str(), table.length());
		sql.add('`');
		sql.add(' ');
		if(data.has(0)) {
			php::array item = data[0];
			int j = -1;
			for(auto i= data.begin(); i!= data.end(); ++i) {
				++j;
				if(j == 0) {
					key_to_buffer(static_cast<php::array&>(i->second), sql);
					std::memcpy(sql.put(8), " VALUES ", 8);
				}else{
					sql.add(',');
				}
				val_to_buffer(i->second, sql);
			}
		}else{
			key_to_buffer(data, sql);
			std::memcpy(sql.put(8), " VALUES ", 8);
			val_to_buffer(data, sql);
		}
		query_(std::move(sql));
		return flame::async(this);
	}
	php::value client::remove(php::parameters& params) {
		if(params.length() < 2 || !params[0].is_string()) {
			throw php::exception("table name is required");
		}
		php::string& table = static_cast<php::string&>(params[0]);
		php::buffer sql;
		std::memcpy(sql.put(13), "DELETE FROM `", 13);
		std::memcpy(sql.put(table.length()), table.c_str(), table.length());
		sql.add('`');
		sql_where(this, params[1], sql);
		if(params.length() > 2) {
			sql_orderby(this, params[2], sql);
		}
		if(params.length() > 3) {
			sql_limit(this, params[3], sql);
		}
		query_(std::move(sql));
		return flame::async(this);
	}
	php::value client::update(php::parameters& params) {
		if(params.length() < 2 || !params[0].is_string() || (!params[2].is_array() && !params[2].is_string())) {
			throw php::exception("table name and update is required");
		}
		php::string& table = static_cast<php::string&>(params[0]);
		
		php::buffer  sql;
		std::memcpy(sql.put(8), "UPDATE `", 8);
		std::memcpy(sql.put(table.length()), table.c_str(), table.length());
		std::memcpy(sql.put(6), "` SET ", 6);
		if(params[2].is_string()) {
			php::string& update = static_cast<php::string&>(params[2]);
			std::memcpy(sql.put(update.length()), update.c_str(), update.length());
		}else{
			php::array&  data  = static_cast<php::array&>(params[2]);
			int j = -1;
			for(auto i=data.begin(); i!=data.end(); ++i) {
				php::string key = i->first.to_string();
				if(++j > 0) sql.add(',');
				sql.add('`');
				std::memcpy(sql.put(key.length()), key.c_str(), key.length());
				sql.add('`');
				sql.add('=');
				val_to_buffer(i->second, sql);
			}
		}
		sql_where(this, params[1], sql);
		if(params.length() > 3) {
			sql_orderby(this, params[3], sql);
		}
		if(params.length() > 4) {
			sql_limit(this, params[4], sql);
		}
		query_(std::move(sql));
		return flame::async(this);
	}
	php::value client::one(php::parameters& params) {
		if(params.length() < 2 || !params[0].is_string()) {
			throw php::exception("table name is required");
		}
		php::string& table = static_cast<php::string&>(params[0]);
		php::buffer  sql;
		std::memcpy(sql.put(15), "SELECT * FROM `", 15);
		std::memcpy(sql.put(table.length()), table.c_str(), table.length());
		sql.add('`');
		sql_where(this, params[1], sql);
		if(params.length() > 2) {
			sql_orderby(this, params[2], sql);
		}
		std::memcpy(sql.put(8), " LIMIT 1", 8);
		client_request_t* ctx = new client_request_t {
			coroutine::current, impl, nullptr, std::move(sql)
		};
		ctx->req.data = ctx;
		impl->worker_.queue_work(&ctx->req, client_implement::one_wk, one_cb);
		return flame::async(this);
	}
	void client::one_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->code != 0) { // 错误信息
			ctx->rv = php::exception::create(ctx->sql, ctx->code);
			ctx->co->next(ctx->rv);
			delete ctx;
		}else if(ctx->rv.is_pointer()) { // ResultSet 且近返回一行
			MYSQL_RES* rs = ctx->rv.ptr<MYSQL_RES>();
			php::array rv(4);
			sql_fetch_row(rs, rv, MYSQL_FETCH_ASSOC);
			ctx->co->next(std::move(rv));
			// MYSQL_RES* 指针需要清理 且这里复用 ctx 上下文
			ctx->self->worker_.queue_work(&ctx->req, client_implement::two_wk, two_cb);
		}
	}
	void client::two_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		delete ctx;
	}
	php::value client::select(php::parameters& params) {
		if(params.length() < 1 || !params[0].is_string()) {
			throw php::exception("table name is required");
		}
		php::string& table = static_cast<php::string&>(params[0]);
		php::buffer  sql;
		std::memcpy(sql.put(7), "SELECT ", 7);
		if(params.length() > 1) {
			if(params[1].is_array()) {
				php::array& col = static_cast<php::array&>(params[1]);
				int j = -1;
				for(auto i=col.begin();i!=col.end();++i) {
					php::string str = i->second;
					if(++j > 0) sql.add(',');
					sql.add('`');
					std::memcpy(sql.put(str.length()), str.c_str(), str.length());
					sql.add('`');
				}
			}else{
				php::string& str = params[1].to_string();
				std::memcpy(sql.put(str.length()), str.c_str(), str.length());
			}
		}else{
			sql.add('*');
		}
		std::memcpy(sql.put(7), " FROM `", 7);
		std::memcpy(sql.put(table.length()), table.c_str(), table.length());
		sql.add('`');
		if(params.length() > 2) {
			sql_where(this, params[2], sql);
		}
		if(params.length() > 3) {
			sql_orderby(this, params[3], sql);
		}
		if(params.length() > 4) {
			sql_limit(this, params[4], sql);
		}
		query_(std::move(sql));
		return flame::async(this);
	}
	void client::default_cb(uv_work_t* req, int status) {
		client_request_t* ctx = reinterpret_cast<client_request_t*>(req->data);
		if(ctx->code != 0) { // 错误信息
			ctx->rv = php::exception::create(ctx->sql, ctx->code);
		}else if(ctx->rv.is_pointer()) { // ResultSet
			MYSQL_RES* rs = ctx->rv.ptr<MYSQL_RES>();
			php::object obj = php::object::create<result_set>();
			result_set* cpp = obj.native<result_set>();
			cpp->init(&ctx->self->worker_, ctx->self->client_, rs);
			ctx->rv = std::move(obj);
		}else{ // ResultInfo
			php::object obj = php::object::create<result_info>();
			result_info* cpp = obj.native<result_info>();
			cpp->init(ctx->rv, ctx->sql);
			ctx->rv = std::move(obj);
		}
		if(ctx->co) {
			ctx->co->next(ctx->rv);
		}
		delete ctx;
	}
}
}
}
