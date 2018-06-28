#include "../coroutine.h"
#include "http.h"
#include "handler.h"
#include "value_body.h"
#include "chunked_writer.h"
#include "file_writer.h"
#include "server_request.h"
#include "server_response.h"

namespace flame {
namespace http {
	void server_response::declare(php::extension_entry& ext) {
		php::class_entry<server_response> class_server_response("flame\\http\\server_response");
		class_server_response
			.property({"status", 200})
			.property({"header", nullptr})
			.property({"cookie", nullptr})
			.property({"body",   nullptr})
			.method<&server_response::__construct>("__construct", {}, php::PRIVATE)
			.method<&server_response::to_string>("__toString")
			.method<&server_response::set_cookie>("set_cookie", {
				
			})
			.method<&server_response::write_header>("write_header", {
				{"status", php::TYPE::INTEGER, false, true}
			})
			.method<&server_response::write>("write", {
				{"chunk", php::TYPE::UNDEFINED}
			})
			.method<&server_response::end>("end", {
				{"chunk", php::TYPE::UNDEFINED, false, true}
			})
			.method<&server_response::file>("file", {
				{"root", php::TYPE::STRING},
				{"path", php::TYPE::STRING},
			});

		ext.add(std::move(class_server_response));
	}
	// 声明为 ZEND_ACC_PRIVATE 禁止创建（不会被调用）
	php::value server_response::__construct(php::parameters& params) {
		return nullptr;	
	}
	php::value server_response::set_cookie(php::parameters& params) {
		php::array cookie(4);
		// TODO set_cookie
		php::array cookies = get("cookie", true);
		if(cookies.typeof(php::TYPE::NULLABLE)) {
			cookies = php::array(4);
		}
		cookies.set(cookies.size(), cookie);
		return nullptr;
	}
	// chunked encoding 用法
	php::value server_response::write_header(php::parameters& params) {
		if(sr_.is_header_done()) {
			throw php::exception(zend_ce_exception, "header already sent");
		}
		if(params.size() > 0) {
			set("status", params[0].to_integer());
		}
		std::make_shared<chunked_writer>(this, flame::coroutine::current)
			->start(chunked_writer::STEP_WRITE_HEADER);
		return coroutine::async();
	}
	php::value server_response::write(php::parameters& params) {
		if(sr_.is_done()) {
			throw php::exception(zend_ce_exception, "response already done");
		}
		std::make_shared<chunked_writer>(this, flame::coroutine::current)
			->start(chunked_writer::STEP_WRITE_CHUNK, params[0]);
		return coroutine::async();
	}
	php::value server_response::end(php::parameters& params) {
		if(sr_.is_done()) {
			throw php::exception(zend_ce_exception, "response already done");
		}
		if(params.size() > 0) {
			std::make_shared<chunked_writer>(this, flame::coroutine::current)
				->start(chunked_writer::STEP_WRITE_CHUNK_LAST, params[0]);
		}else{
			std::make_shared<chunked_writer>(this, flame::coroutine::current)
				->start(chunked_writer::STEP_WRITE_CHUNK_LAST);
		}
		return coroutine::async();
	}
	php::value server_response::file(php::parameters& params) {
		if(sr_.is_done()) {
			throw php::exception(zend_ce_exception, "response already done");
		}
		
		boost::filesystem::path root, file;
		root += params[0].to_string();
		file += params[1].to_string();

		std::make_shared<file_writer>(this, flame::coroutine::current, root / file.lexically_normal())->start();
		return coroutine::async();
	}
	php::value server_response::to_string(php::parameters& params) {
		std::stringstream os;
		os << ctr_;
		return os.str();
	}
	server_response::server_response()
	: sr_(ctr_)
	, status_(0) {

	}
	// 支持 content-length 形式的用法
	void server_response::build_ex() {
		if(status_ & STATUS_ALREADY_BUILT) return; // 重复使用
		status_ |= STATUS_ALREADY_BUILT;
		ctr_.result( get("status").to_integer() ); // 非法 status_code 会抛出异常
		php::array headers = get("header", true);
		if(headers.typeof(php::TYPE::ARRAY)) {
			for(auto i=headers.begin(); i!=headers.end(); ++i) {
				php::string key { i->first };
				i->second.to_string();

				ctr_.set(boost::string_view(key.c_str(), key.size()), i->second);
			}
		}
		ctr_.set(boost::beast::http::field::connection, ctr_.keep_alive() ? "keep-alive" : "close");
		php::array cookies = get("cookie", true);
		if(cookies.typeof(php::TYPE::ARRAY)) {
			for(auto i=cookies.begin(); i!=cookies.end(); ++i) {
				// TODO cookie
				// ctr_.insert("Set-Cookie", i->second);
				std::clog << "cookie: " << i->second << std::endl;
			}
		}
CTYPE_AGAIN:
		auto ctype = ctr_.find(boost::beast::http::field::content_type);
		if(ctype == ctr_.end()) {
			ctr_.set(boost::beast::http::field::content_type, "text/plain");
			goto CTYPE_AGAIN;
		}
		php::string body = get("body");
		if(body.empty()) return;
		ctr_.body() = ctype_encode(ctype->value(), body);
		ctr_.prepare_payload();
	}
}
}
