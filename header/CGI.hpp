#pragma once

#include "Request.hpp"
#include "Response.hpp"

#include <map>
#include <string>
#include <vector>

class CGI
{
	public:
		CGI();
		CGI(CGI const& rhs);
		~CGI();
		CGI & operator=(CGI const& rhs);

		void launchCGI(Request & request, Response & response, Conf & conf);
	
	private:
		std::string scriptFilename;
		std::string requestMethod;
		std::string queryString;
		std::string contentLength;
		std::string contentType;
		std::string uploadDir;

		int fds_out[2];
		std::vector<const char *> args;
		std::vector<const char *> env;

		void initCGIArgs(Response & response, Conf & conf);
		void initCGIEnv(Request const& request, Response & response, Conf const& conf);
		void execCGI(Request & request, Response & response);
};
