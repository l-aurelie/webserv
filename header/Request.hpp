#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#define BAD_REQUEST "400 Bad Request"
#define NOT_FOUND "404 Not Found"

class Request
{
	public:
		Request();
		Request(Request const& rhs);
		~Request();

		Request& operator=(Request const& rhs);

		std::string getMethod() const;
		std::string getPath() const;
		std::string getProtocolVersion() const;
		std::string getServerName() const;
		uint16_t getPort() const;
		std::string getStatusCode() const;

		void setMethod(std::string method);
		void setPath(std::string path);
		void setProtocolVersion(std::string protocolVersion);
		void setHost(std::vector<std::string> & values);
		Request & errorMsg(std::string statusCode, const char * err_msg);

	private:
		std::string method;
		std::string path;
		std::string protocolVersion;
		std::string serverName;
		uint16_t port;
		std::string statusCode;
};

std::ostream & operator<<(std::ostream & os, Request const& rhs);