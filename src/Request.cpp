#include "Request.hpp"
#include "Parser.hpp"
#include <algorithm>
#include <cstdlib>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>

Request::Request() : headerSize(0), headerFilled(false), countContentLength(0), countClientMaxBodySize(0), contentLength(0), port(80) {
	tmpFilename = "/tmp/webserv_XXXXXX";
	int fd = mkstemp(&(*tmpFilename.begin()));
	if (fd != -1)
		close(fd);
	this->tmpFile.open(tmpFilename.c_str(), std::fstream::out | std::fstream::in | std::fstream::binary | std::fstream::trunc);
	if (this->tmpFile.fail())
		std::cerr << "failed to open" << std::endl;	// TODO: error ?
}

Request::Request(Request const& rhs) { *this = rhs; }
Request::~Request() {
//	remove(tmpFilename.c_str());
}

Request& Request::operator=(Request const& rhs) {
	if (this == &rhs)
		return (*this);
	//this->tmpFile = rhs.tmpFile;	// TODO: copy tmp file ?!
	this->tmpFilename = rhs.tmpFilename;	// TODO: copy tmp file ?!
	this->statusCode = rhs.statusCode;
	this->headerSize = rhs.headerSize;
	this->headerFilled = rhs.headerFilled;
	this->headerBuf = rhs.headerBuf;
	this->countContentLength = rhs.countContentLength;
	this->countClientMaxBodySize = rhs.countClientMaxBodySize;

	this->method = rhs.method;
	this->path = rhs.path;
	this->protocolVersion = rhs.protocolVersion;
	this->serverName = rhs.serverName;
	this->contentLength = rhs.contentLength;
	this->contentType = rhs.contentType;
	this->chunked = rhs.chunked;
	this->port = rhs.port;
	return (*this);
}

std::ostream & operator<<(std::ostream & os, Request const& rhs) {
	os << "Request:" << std::endl;
	os << "\tmethod: " << rhs.getMethod() << std::endl;
	os << "\tpath: " << rhs.getPath() << std::endl;
	os << "\tprotocol: " << rhs.getProtocolVersion() << std::endl;

	if (rhs.getServerName().length())
		os << "\tHost: " << rhs.getServerName() << ":" << rhs.getPort() << std::endl;

	if (rhs.getContentLength())
		os << "\tContent-Length: " << rhs.getContentLength() << std::endl;

	if (!rhs.getContentType().empty())
		os << "\tContent-Type: " << rhs.getContentType() << std::endl;
//	if (rhs.getBody().length())
//		os << "\tBody: " << rhs.getBody() <<  std::endl;

	if (rhs.getChunked())
		os << "\tTransfer-Encoding: chunked" << std::endl;

//	os << "\tstatus: " << rhs.errorMsg() << std::endl;
	return (os);
}
/* Setter */

void Request::setMethod(std::string method) { this->method = method; }
void Request::setPath(std::string path) { this->path = path; }
void Request::setProtocolVersion(std::string protocolVersion) { this->protocolVersion = protocolVersion; }
void Request::setHost(std::vector<std::string> & values) {
	if (values.size() != 1)
	{
		std::cerr << "error: request : 'Host' accepts only one host" << std::endl;
		exit(EXIT_FAILURE);	// TODO: error response
	}
//	std::cout << "values[0] = " << values[0] << std::endl;
	std::string delimiter = ":";
	std::size_t colon_position = values[0].find(delimiter);
	if (colon_position == values[0].npos)	// if no ':' then port is 80 by default
	{
		this->serverName = values[0];
		return ;
	}
	this->serverName = values[0].substr(0, colon_position);

	std::stringstream ss(values[0].substr(colon_position + delimiter.length()));
	if (!std::isdigit(ss.str()[0]))
	{
		std::cerr << "error: request : 'Host' port should be between 0 and 65535" << std::endl;
		exit(EXIT_FAILURE);// TODO: error response
	}
	ss >> this->port;
	if (ss.fail())
	{
		std::cerr << "error: request : 'Host' port should be between 0 and 65535" << std::endl;
		exit(EXIT_FAILURE);	// TODO: error response
	}
}

void Request::setContentLength(std::vector<std::string> & values) {
	if (values.size() != 1)
	{
		std::cerr << "error: request : 'Content-Length' accepts only one number" << std::endl;
		exit(EXIT_FAILURE);	// TODO: error response
	}
	std::stringstream ss(values[0]);
	if (!std::isdigit(ss.str()[0]))
	{
		std::cerr << "error: request : 'Content-Length' should be an unsigned number" << std::endl;
		exit(EXIT_FAILURE);// TODO: error response
	}
	ss >> this->contentLength;
	if (ss.fail())
	{
		std::cerr << "error: request : 'Content-Length' should be an unsigned number" << std::endl;
		exit(EXIT_FAILURE);	// TODO: error response
	}
}

void Request::setContentType(std::vector<std::string> & values) {
	for (std::vector< std::string >::iterator it = values.begin(); it != values.end(); ++it)
	{
		this->contentType += *it;
		if (it != values.end() - 1)
			this->contentType += " ";
	}
}

void Request::setTransferEncoding(std::vector<std::string> & values) {
	if (values.size() != 1)
	{
		std::cerr << "error: request : 'Transfer-Encoding' can have at most 1 value" << std::endl;
		exit(EXIT_FAILURE);	// TODO: error response
	}
	if (Parser::tolowerstr(values[0]) == "chunked")
		this->chunked = true;
	else
		this->chunked = false;
}

/* Getter */

std::string Request::getMethod() const { return (this->method); }
std::string Request::getPath() const { return (this->path); }
std::string Request::getProtocolVersion() const { return (this->protocolVersion); }
std::string Request::getServerName() const { return (this->serverName); }
uint16_t Request::getPort() const { return (this->port); }
std::size_t Request::getContentLength() const { return (this->contentLength); }
std::string Request::getContentType() const { return (this->contentType); }
bool Request::getChunked() const { return (this->chunked); }

Request & Request::errorMsg(std::string statusCode, const char * err_msg){
	this->statusCode = statusCode;
	std::cerr << "error: request: " << err_msg << std::endl;
	return (*this);
}
