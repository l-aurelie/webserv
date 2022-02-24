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

		void launchCGI(Request & request, Response & response);
	
	private:
		std::string scriptFilename;
		std::string requestMethod;
		std::string queryString;
		std::string contentLength;
		std::string contentType;

		int fds_out[2];
		std::vector<const char *> args;
		std::vector<const char *> env;

		void initCGIArgs(std::string const& path);
		void initCGIEnv(Request const& request, Response & response);
		void execCGI(Request & request, Response & response);
};
