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

		void setMethod(std::string method);
		void setPath(std::string path);
		void setProtocolVersion(std::string protocolVersion);
		void setHost(std::vector<std::string> & values);

	private:
		std::string method;
		std::string path;
		std::string protocolVersion;
		std::string serverName;
		uint16_t port;
};

std::ostream & operator<<(std::ostream & os, Request const& rhs);