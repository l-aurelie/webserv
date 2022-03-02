curl \
	--verbose \
	--request POST \
	--header 'Transfer-Encoding: chunked' \
	--form 'file_type=txt' \
	--form 'userfile=@./site/bigfile.txt' \
	localhost:6500/upload.php
