#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#define UPLOAD_DIR "/Users/antoine/Documents/GitHub/webserv/upload/"

#define BUF_SIZE 4096

//-- Success
#define NO_CONTENT          "203 No Content"

//-- Redirect
#define MULTIPLE_CHOICES    "300 Multiple Choices"
#define MOVED_PERMANENTLY   "301 Moved Permanently"
#define FOUND               "302 Found"
#define SEE_OTHER           "303 See Other"
#define NOT_MODIFIED        "304 Not Modified"
#define USE_PROXY           "305 Use Proxy"
#define TEMPORARY_REDIRECT  "307 Temporary Redirect"

//-- Client Errors
#define BAD_REQUEST         "400 Bad Request"
#define FORBIDDEN           "403 Forbidden"
#define NOT_FOUND           "404 Not Found"
#define METHOD_NOT_ALLOWED  "405 Method Not Allowed"
#define TOO_LARGE           "413 Request Entity Too Large"

//-- Server Errors
#define INTERNAL            "500 Internal Server Error"


#endif
