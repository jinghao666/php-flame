<?php
ob_start();

flame\init("rabbitmq_1");
flame\go(function() {
	$c = yield flame\rabbitmq\connect("amqp://wuhao:123456@11.22.33.44:5672/vhost");
	$count = 0;
	$consumer = $c->consume("flame-test", function($consumer, $msg) use(&$count) {
		++$count;
		// var_dump($msg);
	 	$consumer->confirm($msg);
	});
	// 这里开启额外的协程执行消费过程
	flame\go(function() use($consumer) {
		yield $consumer->run();
	});
	$producer = $c->produce();
	for($i=0;$i<1000;++$i) {
		$producer->publish(rand(), "flame-test");
		yield flame\time\sleep(5);
		$message = new flame\rabbitmq\message(rand());
		$message->header["a"] = "bb";
		$message->content_type = "text/plain";
		$producer->publish($message, "flame-test");
		yield flame\time\sleep(5);
	}
	yield flame\time\sleep(1000); // 稍微给点时间确认消费完
	yield $consumer->close();

	echo "done:{$count}.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done:2000.\n");
}