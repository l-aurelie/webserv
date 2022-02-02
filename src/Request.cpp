#include "Request.hpp"
#include <algorithm>
#include <stdint.h>
#include <string>
#include <sstream>
#include <iostream>

Request::Request() : port(80) {}
Request::Request(Request const& rhs) { *this = rhs; }
Request::~Request() {}

Request& Request::operator=(Request const& rhs) {
	if (this == &rhs)
		return (*this);
	this->method = rhs.method;
	this->path = rhs.path;
	this->protocolVersion = rhs.protocolVersion;
	this->serverName = rhs.serverName;
	this->port = rhs.port;
	this->statusCode = rhs.statusCode;
	return (*this);
}

std::ostream & operator<<(std::ostream & os, Request const& rhs) {
	os << "Request:" << std::endl;
	os << "\tmethod: " << rhs.getMethod() << std::endl;
	os << "\tpath: " << rhs.getPath() << std::endl;
	os << "\tprotocol: " << rhs.getProtocolVersion() << std::endl;

	if (rhs.getServerName().length())
		os << "\tHost: " << rhs.getServerName() << ":" << rhs.getPort() << std::endl;

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
		std::cerr << "error: request : 'host' accepts only one host" << std::endl;
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

	std::stringstream ss;
	ss << values[0].substr(colon_position + delimiter.length());
	if (!std::isdigit(ss.str()[0]))
	{
		std::cerr << "error: request : 'host' port should be between 0 and 65535" << std::endl;
		exit(EXIT_FAILURE);// TODO: error response
	}
	ss >> this->port;
	if (ss.bad() || ss.fail())
	{
		std::cerr << "error: request : 'host' port should be between 0 and 65535" << std::endl;
		exit(EXIT_FAILURE);	// TODO: error response
	}
}

/* Getter */

std::string Request::getMethod() const { return (this->method); }
std::string Request::getPath() const { return (this->path); }
std::string Request::getProtocolVersion() const { return (this->protocolVersion); }
std::string Request::getServerName() const { return (this->serverName); }
uint16_t Request::getPort() const { return (this->port); }
std::string Request::getStatusCode() const { return this->statusCode; }

Request & Request::errorMsg(std::string statusCode, const char * err_msg){
	this->statusCode = statusCode;
	std::cerr << "error: request: " << err_msg << std::endl;
	return (*this);
}