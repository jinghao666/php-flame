#include "../coroutine.h"
#include "rabbitmq.h"
#include "consumer.h"
#include "message.h"

namespace flame {
namespace rabbitmq {
	void consumer::declare(php::extension_entry& ext) {
		php::class_entry<consumer> class_consumer("flame\\rabbitmq\\consumer");
		class_consumer
			.property({"queue", ""})
			.property({"tag", ""})
			.method<&consumer::__construct>("__construct", {}, php::PRIVATE)
			.method<&consumer::run>("run", {
				{"callable", php::TYPE::CALLABLE},
			})
			.method<&consumer::confirm>("confirm", {
				{"message", "flame\\rabbitmq\\message"}
			})
			.method<&consumer::reject>("reject", {
				{"message", "flame\\rabbitmq\\message"},
				{"requeue", php::TYPE::BOOLEAN, false, true},
			})
			.method<&consumer::close>("close");
		ext.add(std::move(class_consumer));
	}
	php::value consumer::__construct(php::parameters& params) {
		return nullptr;
	}
	php::value consumer::run(php::parameters& params) {
		cb_ = params[0];
		co_ = coroutine::current;
		co_->stack(nullptr, php::value(this));
		std::string queue = get("queue");
		amqp_.channel->consume(queue, flag_)
			// onReceived 等回调不能捕获 php::value(this) 否则会导致该引用被长时间持有
			.onReceived([this] (const AMQP::Message &m, std::uint64_t tag, bool redelivered) {
				php::object msg(php::class_entry<message>::entry());
				message* msg_ = static_cast<message*>(php::native(msg));
				msg_->build_ex(m, tag);
				std::make_shared<coroutine>()->start(cb_, {msg});
			}).onSuccess([this] (const std::string& tag) {
				set("tag", tag);
			}).onError([this] (const char* message) {
				co_->fail(message);
			});
		return coroutine::async();
	}
	php::value consumer::confirm(php::parameters& params) {
		message* msg_ = static_cast<message*>(php::native(params[0]));
		return amqp_.channel->ack(msg_->tag_, 0);
	}
	php::value consumer::reject(php::parameters& params) {
		php::object msg = params[0];
		int flags = 0;
		if(params.size() > 1) {
			if(params[1].to_boolean()) flags |= AMQP::requeue;
		}
		return amqp_.channel->reject(msg.get("tag").to_integer(), flags);
	}
	php::value consumer::close(php::parameters& params) {
		std::string tag = get("tag");
		if(tag.size() > 0) {
			std::shared_ptr<coroutine> co = coroutine::current;
			co->stack(nullptr, php::value(this)); // 保存当前对象引用
			amqp_.channel->cancel(tag).onSuccess([this, co] (const std::string& tag) mutable {
				co_->resume();
				co->resume();
				co.reset();
				co_.reset();
				cb_ = nullptr;
			});
			return coroutine::async();
		}else{
			return nullptr;
		}
	}
}
}
