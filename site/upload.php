<?php
    $uploaddir = '/Users/antoine/Documents/GitHub/webserv/upload/';
    $uploadfile = $uploaddir . basename($_FILES['userfile']['name']);

    echo "<p>";

    if (move_uploaded_file($_FILES['userfile']['tmp_name'], $uploadfile)) {
        echo "File is valid, and was successfully uploaded.\n";
    } else {
        echo "Upload failed";
    }

    echo "</p>";
    echo '<pre>';
    echo 'Here is some more debugging info:';
    print_r($_FILES);

	echo "ENV = ";
	var_dump(getenv());
    print "</pre>";
?>
