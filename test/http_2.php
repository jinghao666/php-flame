<?php
// ob_start();
flame\init("http_2");
flame\go(function() {
	$server = new flame\http\server("127.0.0.1:7678");
	$server->before(function($req, $res, $r) {
		if(!$r) {
			$req->path = "/"; 
		}
	})->get("/", function($req, $res) {
		$res->body = "GET: hello world!\n";
	})->post("/", function($req, $res) {
		var_dump($req);
		yield $res->write("POST: ");
		yield $res->end("hello world!\n");
	})->after(function($req, $res) {
		echo "done1.\n";
	});
	yield $server->run();
});
flame\run();

// if(getenv("FLAME_PROCESS_WORKER")) {
// 	$output = ob_get_flush();
// 	assert($output == "done1.\n");
// }
