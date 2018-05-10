<?php
flame\init("rabbitmq_consumer");
flame\go(function() {
	$consumer = new flame\db\rabbitmq\consumer(
		"amqp://wuhao:123456@11.22.33.44:5672/wuhao",
		[], // options
		"flame-test"); // 队列名 或 ["flame-test"] 允许消费多个队列
	$count = 0;
	$exit  = false;
	$tick = flame\time\after(600000, function() use(&$exit) {
		// 600s 后退出
		$exit = true;
	});
	$begin = microtime(true);
	while(!$exit) {
		$msg = yield $consumer->consume();
		++ $count;
		yield $consumer->confirm($msg); // 消息确认 ack
		// yield $consumer->reject($msg); // 消息丢弃 nack
		// yield $consumer->reject($msg, true); // 消息丢弃 nack 重入队列
		echo "(", $count, ") ", $msg->timestamp(), " => ", $msg, "\n";
	}
	$end = microtime(true);
	echo $count, " messages consumed in ", $end - $begin, "s\n";
	$tick->stop();
});
flame\run();
