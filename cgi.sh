#! /bin/zsh

#echo SHLVL

export GATEWAY_INTERFACE="CGI/1.1";
export REDIRECT_STATUS="true";												# GET do not work without
export SCRIPT_FILENAME="/home/aurelie/Documents/webserv/site///phpinfo.php";	# script that php-cgi call even if not in arg
export REQUEST_METHOD="GET";													# set GET or PORT method
export QUERY_STRING="toto=6&send=send";												# this is where GET values are set

env
echo ""
echo ""
echo ""
echo ""
#export CONTENT_LENGTH="500";													# only for POST
#export CONTENT_TYPE="application/x-www-form-urlencoded";													# only for POST
echo "title=aaaaaaa&info=je suis antoine&allergens=lait&unitPrice=5.90&unitUnity=euros&lotPrice=12.90&lotUnity=euros&add=added" | ./cgi/php-cgi;
#./cgi/php-cgi
#/home/aurelie/Documents/webserv/site//../cgi/php-cgi /home/aurelie/Documents/webserv/site///phpinfo.php