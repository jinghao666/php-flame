<?php
ob_start();

flame\init("rabbitmq_1");
flame\go(function() {
	$client = yield flame\rabbitmq\connect("amqp://wuhao:123456@11.22.33.44:5672/vhost");
	$count = 0;
	$consumer = $client->consume("flame-test");
	flame\time\after(3000, function() use($consumer) {
		yield $consumer->close();
	});
	yield $consumer->run(function($msg) use(&$count, $consumer) {
		++$count;
		// var_dump($msg);
	 	$consumer->confirm($msg);
	});

	echo "done:{$count}.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done:200.\n");
}