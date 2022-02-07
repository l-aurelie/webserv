#pragma once

#include <stdint.h>
#include <string>
#include <vector>

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
		std::size_t getContentLength() const;
		std::string getBody() const;

		void setBody();
		void setMethod(std::string method);
		void setPath(std::string path);
		void setProtocolVersion(std::string protocolVersion);
		void setHost(std::vector<std::string> & values);
		void setContentLength(std::vector<std::string> & values);

		Request & errorMsg(std::string statusCode, const char * err_msg);

		std::string buffer;
		std::string body;
		std::string statusCode;
		std::size_t headerSize;

	private:
		std::string method;
		std::string path;
		std::string protocolVersion;
		std::string serverName;
		std::size_t contentLength;
		uint16_t port;
};

std::ostream & operator<<(std::ostream & os, Request const& rhs);