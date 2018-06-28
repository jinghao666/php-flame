## Flame
**Flame** 是一个 PHP 框架，借用 PHP Generator 实现 协程式 的编程服务。目前，flame 中提供了如下功能：
1. 核心功能
	1. [核心协程函数](/php-flame/core)；
	2. [时间协程函数](/php-flame/time) - 毫秒时间戳, 调度休眠、定时器；
	3. [操作系统函数](/php-flame/os) - 网卡信息, 异步进程；
	4. [日志输出](/php-flame/log)；
2. 网络
	<!-- 1. [辅助函数](/php-flame/flame_net)； -->
	2. [HTTP 客户端、处理器](/php-flame/http)；
	<!-- 3. [Unix Socket 客户端、服务端](/php-flame/flame_net)； -->
	<!-- 4. [TCP 客户端、服务端](/php-flame/flame_net)； -->
	<!-- 5. [UDP 客户端、服务端](/php-flame/flame_net)； -->
3. 驱动
	1. [Redis 客户端](/php-flame/redis)；
	<!-- 2. [Mongodb 客户端](/php-flame/flame_db_mongodb) - 简单封装； -->
	<!-- 3. [MySQL 客户端](/php-flame/flame_db_mysql) - 简单封装； -->
	<!-- 4. [Kafka](/php-flame/flame_db_kafka) - 简单生产消费； -->
	4. [RabbitMQ](/php-flame/rabbitmq)；

**开源仓库**：
[https://github.com/terrywh/php-flame/](https://github.com/terrywh/php-flame/)

**注意事项**：
* 本文档函数说明前置 `yield` 关键字, 标识此函数为 "**异步函数**";
* 调用异步函数需要使用 `yield fn(...);` 形式；
* 存在异步函数调用的封装函数也是异步函数 (也需要 `yield fn(...);` 形式调用)；
* 使用 flame\go() 启动第一个"异步函数";
* 构造、析构函数等特殊函数无法定义为异步函数；(无法在构造或析构函数中调用异步函数)

**示例**：
``` PHP
<?php
// 框架初始化（自动设置进程名称）
flame\init("http-server", [
	"worker" => 4, // 多进程服务
]);
// 启用一个协程作为入口
flame\go(function() {
	// 创建 http 服务器
	$server = new flame\http\server();
	// 设置默认处理程序
	$server->before(function($req, $res, &$next) {
		if($next) {
			$req->data["time"] = flame\time\now();
		}
	})->get("/hello", function($req, $res) {
		$res->body = "world";
	})->post("/hello", function($req, $res) {
		yield $res->write_header(404);
		yield $res->write("chunk1");
		yield $res->end("chunk2");
	})->after(function($req, $res, $next) {
		if($next) {
			flame\log\info("request '/hello' elapsed: ", flame\time\now() - $req->data["time"], "ms");
		}
	});
	// 启动并持续运行
	yield $server->run();
});
// 框架调度执行
flame\run();
```

完整可参考示例: [examples](https://github.com/terrywh/php-flame/tree/master/examples) 下相关代码；
