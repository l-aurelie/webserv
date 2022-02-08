<?php

echo "<h1>call to phpinfo.php\n</h1>";


echo "<pre>";
echo "GET = ";
var_dump($_GET);

echo "POST = ";
var_dump($_POST);
echo "</pre>";

echo "<p>";
for ($i=0; $i < 5; $i++) {
	echo "$i Hello World!<br />\n";
}
echo "</p>";

?>