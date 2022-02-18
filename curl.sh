curl \
	--verbose \
	--request POST \
	--header 'Transfer-Encoding: chunked' \
	--form 'userfile=@./site/img.jpeg' \
	localhost:6500/upload.php
