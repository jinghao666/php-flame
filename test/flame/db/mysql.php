<?php
flame\init("mysql_test");

flame\go(function() {
	$cli = new flame\db\mysql\client();
	yield $cli->connect("mysql://wuhao:123456@11.22.33.44:3336/mysql_test");
	var_dump( $cli->format("SELECT * FROM `test_0` WHERE `id`=?", true) );
	for($i=0;$i<1000;++$i) {
		yield $cli->insert("test_0", ["id"=>NULL, "text"=>rand()]);
	}
	var_dump( yield $cli->delete("test_0", [
		"text" => ['$ne'=>"123"]
	], ["text"=>-1], 1) );

	for($i=0;$i<1000;++$i) {
		// $rs = yield $cli->select("test_0", ["id","text"], ["id"=>['$ne'=>NULL]], ["text"=>-1], 30);
		$rs = yield $cli->query("SELECT * FROM `test_0` WHERE `id` IS NOT NULL ORDER BY `text` DESC LIMIT 30");
		$rw = yield $rs->fetch_all(flame\db\mysql\FETCH_ASSOC);
	}
	debug_zval_dump( $rs );
	debug_zval_dump( $rw );
	var_dump( yield $cli->one("test_0", ["id"=>['$gt'=>10000]], ["id"=>1], 1) );
	yield $cli->delete("test_0", "true");
});
flame\run();
