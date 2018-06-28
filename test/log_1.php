<?php
ob_start();

$file = "/tmp/flame.log";
if(!getenv("FLAME_PROCESS_WORKER")) {
	@unlink($file); // 测试清晰起见
}

flame\init("log_1", [
	"logger" => $file,
]);
flame\go(function() use($file) {
	$data = "info logger (". rand() .")";
	$expt = $data."\n";
	flame\log\info($data);
	foreach (file($file) as $line) {
		assert( substr($line, -strlen($expt)) == $expt);
	}
	@unlink($file);
	flame\log\rotate($data);
	// 由于 rotate 通知日志线程执行, 文件不会立刻重新打开
	yield flame\time\sleep(10);
	flame\log\info($data);
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
