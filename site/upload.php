<?php
	phpinfo(INFO_ENVIRONMENT);
	if (isset($_SERVER['UPLOAD_DIR']) && !empty($_SERVER['UPLOAD_DIR']))
	{
		$uploadfile = $_SERVER['UPLOAD_DIR'] . basename($_FILES['userfile']['name']);

		echo "<p>";
		if (move_uploaded_file($_FILES['userfile']['tmp_name'], $uploadfile)) {
			echo "File is valid, and was successfully uploaded.\n";
		} else {
			echo "Upload failed";
		}
		echo "</p>";
	}
	else
	{
		echo "Please specify upload directory";
	}

	echo '<pre>';
	print_r($_FILES);
	print "</pre>";
?>
