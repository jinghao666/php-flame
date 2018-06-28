<?php
// ob_start();
flame\init("http_3");
flame\go(function() {
	$server = new flame\http\server("127.0.0.1:7678");
	$server->after(function($req, $res, $r) {
		if(!$r) { // 未匹配自定义路径处理器
			yield $res->header["content-type"] = "text/html";
			yield $res->file(__DIR__, $req->path);
		}
	});
	yield $server->run();
});
flame\run();

// if(getenv("FLAME_PROCESS_WORKER")) {
// 	$output = ob_get_flush();
// 	assert($output == "done1.\n");
// }
