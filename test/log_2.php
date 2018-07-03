<?php
ob_start();

$file = "/tmp/flame.log";
if(!getenv("FLAME_PROCESS_WORKER")) {
	@unlink($file); // 测试清晰起见
}

flame\init("log_2");
flame\go(function() use($file) {
	$logger = new flame\log\logger($file);
	$data = "info logger (". rand() .")";
	$expt = $data."\n";
	yield flame\time\sleep(10);
	$logger->info($data);
	// 日志写入过程是异步的, 需要稍微等一会
	yield flame\time\sleep(10);
	foreach (file($file) as $line) {
		assert( substr($line, -strlen($expt)) == $expt);
	}
	@unlink($file);
	$logger->rotate($data);
	// 由于 rotate 通知日志线程执行, 文件不会立刻重新打开
	yield flame\time\sleep(10);
	$logger->info($data);
	foreach (file($file) as $line) {
		assert( substr($line, -strlen($expt)) == $expt);
	}
	echo "done1.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done1.\n");
}
