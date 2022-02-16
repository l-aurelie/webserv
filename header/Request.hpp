#pragma once

#include <fstream>
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
		std::string getContentType() const;

		void setMethod(std::string method);
		void setPath(std::string path);
		void setProtocolVersion(std::string protocolVersion);
		void setHost(std::vector<std::string> & values);
		void setContentLength(std::vector<std::string> & values);
		void setContentType(std::vector<std::string> & values);

		Request & errorMsg(std::string statusCode, const char * err_msg);

		std::fstream tmpFile;
		std::string tmpFilename;
		std::string statusCode;
		std::size_t headerSize;
		bool headerFilled;
		std::string headerBuf;

	private:
		std::string method;
		std::string path;
		std::string protocolVersion;
		std::string serverName;
		std::size_t contentLength;
		std::string contentType;
		uint16_t port;
};

std::ostream & operator<<(std::ostream & os, Request const& rhs);
