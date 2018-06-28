<?php
flame\init("core_0");
function async_fn() {
    echo "1\n";
    yield flame\trigger_error();
    yield 1;
    echo "2\n";
    yield 2;
}
flame\go(function() {
	flame\trigger_error();
	yield async_fn();
});
flame\run();
