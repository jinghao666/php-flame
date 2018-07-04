<?php
// ob_start();

flame\init("mysql_1");
flame\go(function() {
	$client = yield flame\mongodb\connect("mongodb://notify:wzysy2dcaateSjcb2rlY@10.20.6.71:27017,10.20.6.72:27017/relation_dev?replicaSet=devel_repl&readPreference=secondaryPreferred");
	var_dump($client);

	$reply = yield $client->command(["count" => "rel-gag"]);
	var_dump($reply);
	$cursor = yield $client->command(["find" => "rel-gag", "filter"=>["aid"=>"94913636"]]);
	var_dump($cursor);
	var_dump(yield $cursor->__toArray());
});
flame\run();

if(getenv("FLAME_PROCESS_WORKER")) {
	// $output = ob_get_flush();
	// assert($output == "done1.\n");
}