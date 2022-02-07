#echo SHLVL
export REQUEST_METHOD="GET";													# set GET or PORT method
export QUERY_STRING="toto=6"												# this is where GET values are set
export SCRIPT_FILENAME="/home/aurelie/Documents/webserv/site/phpinfo.php";	# script that php-cgi call even if not in arg
export REDIRECT_STATUS="true";												# GET do not work without

export CONTENT_LENGTH="500";													# only for POST
export CONTENT_TYPE="application/x-www-form-urlencoded";													# only for POST
export GATEWAY_INTERFACE="CGI/1.1";
#echo "title=aaaaaaa&info=je suis antoine&allergens=lait&unitPrice=5.90&unitUnity=euros&lotPrice=12.90&lotUnity=euros&add=added" | ./cgi/php-cgi;
./cgi/php-cgi