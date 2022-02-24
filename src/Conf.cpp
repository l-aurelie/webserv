#include "Conf.hpp"

#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdint.h>
#include <vector>

Conf::Conf(void) : listen(80), autoindex(-1), clientMaxBodySize(-1), redirectCode(-1) {}
Conf::Conf(Conf const& rhs) { *this = rhs; }
Conf::~Conf(void) {}

Conf& Conf::operator=(Conf const& rhs)
{
	if (this == &rhs)
		return (*this);
	this->listen = rhs.listen;
	this->serverName = rhs.serverName;
	this->autoindex = rhs.autoindex;
	this->index = rhs.index;
	this->root = rhs.root;
	this->clientMaxBodySize = rhs.clientMaxBodySize;
	this->locations = rhs.locations;
	this->redirectCode = rhs.redirectCode;
	this->redirectURL = rhs.redirectURL;
	this->errorPages = rhs.errorPages;
	this->allowedMethods = rhs.allowedMethods;
	this->uploadDir = rhs.uploadDir;
	this->cgi = rhs.cgi;
	return (*this);
}

std::ostream & operator<<(std::ostream & os, Conf const& rhs)
{
	os << "Server {" << std::endl;

	if (rhs.listen)
		os << "\tlisten: " << rhs.listen << ";" << std::endl;
	if (rhs.serverName.size())
	{
		os << "\tserver_name:";
		for (std::vector<std::string>::const_iterator it = rhs.serverName.begin(); it != rhs.serverName.end(); ++it)
			os << " " << *it;
		os << ";" << std::endl;
	}
	if (!rhs.index.empty())
	{
		os << "\tindex:";
		for (std::vector<std::string>::const_iterator it = rhs.index.begin(); it != rhs.index.end(); ++it)
			os << " " << *it;
		os << ";" << std::endl;
	}
	if (!rhs.root.empty())
		os << "\troot: " << rhs.root << ";\n";
	if (rhs.autoindex)
		os << "\tautoindex: on;" << std::endl;
	if (rhs.clientMaxBodySize)
		os << "\tclientMaxBodySize: " << rhs.clientMaxBodySize << ";\n";
	for(std::map<std::string, Conf>::const_iterator it = rhs.locations.begin(); it != rhs.locations.end(); it++)
		std::cout << "location " << it->first << " " << it->second;
	for (std::map< int, std::string >::const_iterator it = rhs.errorPages.begin(); it != rhs.errorPages.end(); it++)
		std::cout << "error_page " << it->first << " " << it->second;
	if (!rhs.allowedMethods.empty())
	{
		os << "\tallowedMethods:";
		for (std::vector<std::string>::const_iterator it = rhs.allowedMethods.begin(); it != rhs.allowedMethods.end(); ++it)
			os << " " << *it;
		os << ";" << std::endl;
	}
	os << "}" << std::endl;
	return (os);
}

//================================================================//

void Conf::setServerName(std::vector<std::string> const& values) { this->serverName = values; }
void Conf::setIndex(std::vector<std::string> const& values) { this->index = values; }

void Conf::setListen(std::vector<std::string> const& values)
{
	if (values.size() != 1)
	{
		std::cerr << "error: config file : 'listen' accepts only one port" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (!std::isdigit(values[0][0]))
	{
		std::cerr << "error: config file : 'listen' unsigned numeric value required" << std::endl;
		exit(EXIT_FAILURE);
	}
	std::stringstream ss;
	ss << values[0];
	ss >> this->listen;
	if (ss.fail())
	{
		std::cerr << "error: config file : 'listen' unsigned numeric value required" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void Conf::setAutoindex(std::vector<std::string> const& values)
{
	if (values.size() != 1)
	{
		std::cerr << "error: config file : 'autoindex' accepts only one value" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (values[0] == "on")
		this->autoindex = true;
	else if (values[0] == "off")
		this->autoindex = false;
	else
	{
		std::cerr << "error: config file : 'autoindex' should be only 'on' or 'off'" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void Conf::setRoot(std::vector<std::string> const& values)
{
	if (values.size() != 1)
	{
		std::cerr << "error: config file : 'root' accepts only one path" << std::endl;
		exit(EXIT_FAILURE);
	}
	this->root = values[0];
}

void Conf::setClientMaxBodySize(std::vector<std::string> const& values)
{
	if (values.size() != 1)
	{
		std::cerr << "error: config file : 'max_client_body_size' accepts only one value" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (!std::isdigit(values[0][0]))
	{
		std::cerr << "error: config file : 'max_client_body_size' unsigned numeric value required" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::stringstream ss;
	ss << values[0];
	ss >> this->clientMaxBodySize;
	if (ss.fail())
	{
		std::cerr << "error: config file : 'max_client_body_size' unsigned numeric value required" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void Conf::setReturn(std::vector<std::string> const& values)
{
	if (values.size() != 2)
	{
		std::cerr << "error: config file : 'return' need code and URL" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (!std::isdigit(values[0][0]))
	{
		std::cerr << "error: config file : 'return' unsigned numeric code required" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::stringstream ss;
	ss << values[0];
	ss >> this->redirectCode;
	if (ss.fail())
	{
		std::cerr << "error: config file : 'return' unsigned numeric code required" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (this->redirectCode < 300 || this->redirectCode > 307 || this->redirectCode == 306)
	{
		std::cerr << "error: config file : 'return' only 300 to 307 (306 excluded) code are implemented" << std::endl;
		exit(EXIT_FAILURE);
	}
	this->redirectURL = values[1];
}

void Conf::setErrorPages(std::vector<std::string> const& values)
{
	if (values.size() != 2)
	{
		std::cerr << "error: config file : 'error_page' need code and path" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (!std::isdigit(values[0][0]))
	{
		std::cerr << "error: config file : 'error_page' unsigned numeric code required" << std::endl;
		exit(EXIT_FAILURE);
	}

	int code;
	std::stringstream ss;
	ss << values[0];
	ss >> code;
	if (ss.fail())
	{
		std::cerr << "error: config file : 'error_page' unsigned numeric code required" << std::endl;
		exit(EXIT_FAILURE);
	}
	this->errorPages[code] = values[1];
}

void Conf::setAllowedMethods(std::vector<std::string> const& values)
{
	this->allowedMethods.clear();
	for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it)
	{
		if (*it != "GET" && *it != "POST" && *it != "DELETE")
		{
			std::cerr << "error: config file : 'allowed_methods' only GET, POST and DELETE are implemented" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	this->allowedMethods = values;
}

void Conf::setUploadDir(std::vector<std::string> const& values)
{
	if (values.size() != 1)
	{
		std::cerr << "error: config file : 'upload_dir' accepts only one path" << std::endl;
		exit(EXIT_FAILURE);
	}
	this->uploadDir = values[0];
}

void Conf::setCGI(std::vector<std::string> const& values)
{
	if (values.size() != 2)
	{
		std::cerr << "error: config file : 'cgi' must have one extension and one path" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (values[0][0] != '.')
	{
		std::cerr << "error: config file : 'cgi' extension must start with a '.'" << std::endl;
		exit(EXIT_FAILURE);
	}
	this->cgi[values[0]] = values[1];
}
