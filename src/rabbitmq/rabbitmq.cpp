#include "../coroutine.h"
#include "rabbitmq.h"
#include "client.h"
#include "consumer.h"
#include "producer.h"
#include "message.h"

namespace flame {
namespace rabbitmq {
	void declare(php::extension_entry& ext) {
		ext
			.function<connect>("flame\\rabbitmq\\connect", {
				{"address", php::TYPE::STRING},
				{"options", php::TYPE::ARRAY, false, true},
			});
		client::declare(ext);
		consumer::declare(ext);
		producer::declare(ext);
		message::declare(ext);
	}
	php::value connect(php::parameters& params) {
		php::object cli(php::class_entry<client>::entry());
		client* cli_ = static_cast<client*>(php::native(cli));
		
		cli_->amqp_.handler.reset(new AMQP::LibBoostAsioHandler(context));
		php::string addr = params[0];
		cli_->amqp_.connection.reset(new AMQP::TcpConnection(
			cli_->amqp_.handler.get(), AMQP::Address(addr.c_str(), addr.size())
		));
		cli_->amqp_.channel.reset(new AMQP::TcpChannel(cli_->amqp_.connection.get()));
		std::shared_ptr<coroutine> co = coroutine::current;
		std::uint16_t prefetch = 1024;
		if(params.size() > 1) {
			php::array opts = params[1];
			if(opts.exists("prefetch")) {
				prefetch = std::max(std::uint16_t(0), std::min(std::uint16_t(opts.get("prefetch").to_integer()), std::uint16_t(65536)));
			}
		}
		cli_->amqp_.channel->setQos(prefetch);
		cli_->amqp_.channel->onReady([co, cli] () mutable {
			co->resume(std::move(cli)); // 需要 move 否则 cli 引用会保留在此回调中
		});
		cli_->amqp_.channel->onError([co] (const char* message) {
			co->fail(message);
		});
		return coroutine::async();
		// return cli;
	}
	// 仅支持一维数组
	php::array convert(const AMQP::Table& table) {
		php::array data(table.size());
		for(auto key : table.keys()) {
			const AMQP::Field& field = table.get(key);
			if(field.isBoolean()) {
				data.set(key, static_cast<uint8_t>(field) ? true : false);
			}else if(field.isInteger()) {
				data.set(key, static_cast<int64_t>(field));
			}else if(field.isDecimal()) {
				data.set(key, static_cast<double>(field));
			}else if(field.isString()) {
				data.set(key, (const std::string&)field);
			}else{ // 其他类型暂不支持
				// TODO 记录警告信息?
			}
		}
		return std::move(data);
	}
	// 仅支持一维数组
	AMQP::Table convert(const php::array& table) {
		AMQP::Table data;
		for(auto i=table.begin(); i!=table.end(); ++i) {
			if(i->second.typeof(php::TYPE::BOOLEAN)) {
				data.set(i->first.to_string(), i->second.to_boolean());
			}else if(i->second.typeof(php::TYPE::INTEGER)) {
				data.set(i->first.to_string(), i->second.to_integer());
			}else if(i->second.typeof(php::TYPE::STRING)) {
				data.set(i->first.to_string(), i->second.to_string());
			}else{
				throw php::exception(zend_ce_type_error, "table conversion failed: unsupported type");
			}
		}
		return std::move(data);
	}
}
}