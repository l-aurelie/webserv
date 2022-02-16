<?php

echo "<h1>call to phpinfo.php\n</h1>";

phpinfo(INFO_ENVIRONMENT);
phpinfo(INFO_VARIABLES);

$uploaddir = '/Users/antoine/Documents/GitHub/webserv/upload/';
$uploadfile = $uploaddir . basename($_FILES['userfile']['name']);
if (move_uploaded_file($_FILES['userfile']['tmp_name'], $uploadfile)) {
    echo "Le fichier est valide, et a été téléchargé
           avec succès. Voici plus d'informations :\n";
}else {
	echo "Attaque potentielle par téléchargement de fichiers.
		  Voici plus d'informations :\n";
}

echo "<p>";
for ($i=0; $i < 5; $i++) {
	echo "$i Hello World!<br />\n";
}
echo "</p>";

?>
