#include "../coroutine.h"
#include "_client.h"
#include "_command_base.h"
#include "_command.h"
#include "client.h"

namespace flame {
namespace redis {
	void client::declare(php::extension_entry& ext) {
		php::class_entry<client> class_client("flame\\redis\\client");
		class_client
			.method<&client::__construct>("__construct", {}, php::PRIVATE)
			.method<&client::__call>("__call", {
				{"cmd", php::TYPE::STRING},
				{"arg", php::TYPE::ARRAY},
			})
			.method<&client::mget>("mget", {
				{"key", php::TYPE::STRING},
			})
			.method<&client::hmget>("hmget", {
				{"hash", php::TYPE::STRING},
			})
			.method<&client::hgetall>("hgetall", {
				{"hash", php::TYPE::STRING},
			})
			.method<&client::hscan>("hscan", {
				{"hash", php::TYPE::STRING},
				{"cursor", php::TYPE::INTEGER},
			})
			.method<&client::zscan>("zscan", {
				{"zset", php::TYPE::STRING},
				{"cursor", php::TYPE::INTEGER},
			})
			.method<&client::zrange>("zrange", {
				{"zset", php::TYPE::STRING},
				{"start", php::TYPE::INTEGER},
				{"stop", php::TYPE::INTEGER},
			})
			.method<&client::zrevrange>("zrevrange", {
				{"zset", php::TYPE::STRING},
				{"start", php::TYPE::INTEGER},
				{"stop", php::TYPE::INTEGER},
			})
			.method<&client::zrangebyscore>("zrangebyscore", {
				{"zset", php::TYPE::STRING},
				{"min", php::TYPE::STRING},
				{"max", php::TYPE::STRING},
			})
			.method<&client::zrevrangebyscore>("zrevrangebyscore", {
				{"zset", php::TYPE::STRING},
				{"min", php::TYPE::STRING},
				{"max", php::TYPE::STRING},
			})
			.method<&client::unsubscribe>("unsubscribe")
			.method<&client::punsubscribe>("punsubscribe")
			// 以下暂未实现
			.method<&client::subscribe>("subscribe")
			.method<&client::subscribe>("psubscribe");
		ext.add(std::move(class_client));
	}
	client::client()
	: cli_(new _client()) {

	}
	php::value client::__construct(php::parameters& params) {
		return nullptr;
	}
	php::value client::__call(php::parameters& params) {
		cli_->send(coroutine::current, new _command(params[0], params[1]));
		return coroutine::async();
	}
	php::value client::mget(php::parameters& params) {
		cli_->send(coroutine::current, new _command("MGET", params, _command_base::REPLY_COMBINE_1));
		return coroutine::async();
	}
	php::value client::hmget(php::parameters& params) {
		cli_->send(coroutine::current, new _command("HMGET", params, _command_base::REPLY_COMBINE_2));
		return coroutine::async();
	}
	php::value client::hgetall(php::parameters& params) {
		cli_->send(coroutine::current, new _command("HGETALL", params, _command_base::REPLY_ASSOC_ARRAY_1));
		return coroutine::async();
	}
	php::value client::hscan(php::parameters& params) {
		cli_->send(coroutine::current, new _command("HSCAN", params, _command_base::REPLY_ASSOC_ARRAY_2));
		return coroutine::async();
	}
	php::value client::zscan(php::parameters& params) {
		cli_->send(coroutine::current, new _command("ZSCAN", params, _command_base::REPLY_ASSOC_ARRAY_2));
		return coroutine::async();
	}
	php::value client::zrange(php::parameters& params) {
		php::string larg = params[params.size() - 1];
		if(larg.typeof(php::TYPE::STRING) && larg.size() == 10 && strncasecmp("WITHSCORES", larg.c_str(), 10) == 0) {
			cli_->send(coroutine::current, new _command("ZRANGE", params, _command_base::REPLY_ASSOC_ARRAY_1));
		}else{
			cli_->send(coroutine::current, new _command("ZRANGE", params));
		}
		return coroutine::async();
	} 
	php::value client::zrevrange(php::parameters& params) {
		php::string arg = params[params.size() - 1];
		if(arg.typeof(php::TYPE::STRING) && arg.size() == 10 && strncasecmp("WITHSCORES", arg.c_str(), 10) == 0) {
			cli_->send(coroutine::current, new _command("ZREVRANGE", params, _command_base::REPLY_ASSOC_ARRAY_1));
		}else{
			cli_->send(coroutine::current, new _command("ZREVRANGE", params));
		}
		return coroutine::async();
	}
	php::value client::zrangebyscore(php::parameters& params) {
		for(int i=3; i<params.size(); ++i) {
			if(params[i].typeof(php::TYPE::STRING) && params[i].size() == 10) {
				php::string arg = params[i];
				if(strncasecmp("WITHSCORES", arg.c_str(), 10) == 0) {
					cli_->send(coroutine::current, new _command("ZRANGEBYSCORE", params, _command_base::REPLY_ASSOC_ARRAY_1));
					return coroutine::async();
				}
			}
		}
		cli_->send(coroutine::current, new _command("ZRANGEBYSCORE", params));
		return coroutine::async();
	}
	php::value client::zrevrangebyscore(php::parameters& params) {
		for(int i=3; i<params.size(); ++i) {
			if(params[i].typeof(php::TYPE::STRING) && params[i].size() == 10) {
				php::string arg = params[i];
				if(strncasecmp("WITHSCORES", arg.c_str(), 10) == 0) {
					cli_->send(coroutine::current, new _command("ZREVRANGEBYSCORE", params, _command_base::REPLY_ASSOC_ARRAY_1));
					return coroutine::async();
				}
			}
		}
		cli_->send(coroutine::current, new _command("ZREVRANGEBYSCORE", params));
		return coroutine::async();
	}
	php::value client::unsubscribe(php::parameters& params) {
		cli_->send(coroutine::current, new _command("ZREVRANGEBYSCORE", params, _command_base::REPLY_ASSOC_ARRAY_1));
		return coroutine::async();
	}
	php::value client::punsubscribe(php::parameters& params) {
		cli_->send(coroutine::current, new _command("ZREVRANGEBYSCORE", params, _command_base::REPLY_ASSOC_ARRAY_1));
		return coroutine::async();
	}
	php::value client::subscribe(php::parameters& params) {
		throw php::exception(zend_ce_error, "subscribe not implemented");
	}
}
}
