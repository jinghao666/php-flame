## Flame
**Flame** 是一个 PHP 框架，借用 PHP Generator 实现**协程式**的程序开发。

## 文档
https://terrywh.github.io/php-flame/


## 示例
``` PHP
<?php
// 框架初始化（自动设置进程名称）
flame\init("http-server", [
	"worker" => 4, // 多进程服务
]);
// 启用一个协程作为入口
flame\go(function() {
	// 创建 http 服务器
	$server = new flame\http\server(":::19001");
	// 设置处理过程
	$server->before(function($req, $res, $r) {
		if(!$r) {
			yield $res->write_header(404);
			yield flame\time\sleep(2000);
			yield $res->end("not found");
		}
	})->get("/hello", function($req, $res) {
		$res->body = "hello, world!\n";
	});
	// 运行服务
	yield $server->run();
});
// 框架调度执行
flame\run();
```

## 依赖

* [boost](https://www.boost.org/)
```
./bootstrap.sh --with-toolset=clang --prefix=/data/vendor/boost-1.67.0
./b2 --prefix=/data/vendor/boost-1.67.0 cxxflags=-fPIC toolset=clang variant=release link=static threading=multi install
```

* [libphpext](https://github.com/terrywh/libphpext.git) - 依赖于 PHP
```
make VENDOR_PHP=/data/vendor/php-x.x.x
make PREFIX=/data/vendor/libphpext-x.x.x install
```

* [cpp-parser](https://github.com/terrywh/cpp-parser.git) - 纯头文件库
```
make PREFIX=/data/vendor/parser-x.x.x install
```

* [AMQP-CPP](https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git) - 仅编译静态库
```
make CPP="clang++" CPPFLAGS="-fPIC -Wall -c -I../include -std=c++11 -MD" static
make PREFIX="/data/vendor/amqp-3.1.0" install
rm -f /data/vendor/amqp-3.1.0/lib/libamqpcpp.so*
```

* [MySQL-Connector-C](https://downloads.mysql.com/archives/c-c/) - 6.1.11 Linux - Generic 64bit
