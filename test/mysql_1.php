<?php
ob_start();

flame\init("mysql_1");
flame\go(function() {
	$client = yield flame\mysql\connect("mysql://user:pass@11.22.33.44:3336/db_name");
	$rs = yield $client->query("SELECT * FROM `test_0`");
	assert(count($rs->fetch_all()) > 0);
	echo "done1.\n";
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	$output = ob_get_flush();
	assert($output == "done1.\n");
}