<?php

echo "call to phpinfo.php\n";

echo "GET = ";
var_dump($_GET);

echo "POST = ";
var_dump($_POST);

for ($i=0; $i < 5; $i++) {
	echo "$i Hello World!\n";
}

?>