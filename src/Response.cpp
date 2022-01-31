#include "Conf.hpp"
#include "Response.hpp"
#include <cstring>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

bool Response::selectConf(std::vector<Conf> & confs, Request & request, Conf & ret) const {
	for (std::vector<Conf>::const_iterator it = confs.begin(); it != confs.end(); ++it)
	{
		std::vector<std::string> server_names = it->getServerName();
		for (std::vector<std::string>::const_iterator it2 = server_names.begin(); it2 != server_names.end(); ++it2)
		{
			if (*it2 == request.getServerName())
			{
				ret = *it;
				return (true);
			}
		}
	}
	return (false);
}

std::string Response::prepareResponse(Request & request, std::vector<Conf> & confs){
	std::stringstream ss;
	Conf conf;
	if (!selectConf(confs, request, conf))
	{
		std::cerr << "error: no corresponding conf found" << std::endl;	// Impossible ? / set status instead
		exit(EXIT_FAILURE);
	}
	fillBody(request, conf);
	fillHeader();

	ss << this->protocolVersion << ' ' << this->statusCode << '\n'; // status line
	ss << "Server: " << this->server << '\n';
	ss << "Content-Length: " << this->contentLength << '\n';
	ss << "Content-Type: " << this->contentType << '\n';

	ss << '\n';

	ss << this->body;
	
	return ss.str();
}

Response::Response() {}	// TODO:
Response::Response(Response const& rhs) { *this = rhs; }
Response::~Response() {}

Response &Response::operator=(Response const& rhs) {
	if (this == &rhs)
		return (*this);
	this->protocolVersion = rhs.protocolVersion;
	this->statusCode = rhs.statusCode;
	this->contentLength = rhs.contentLength;
	this->contentType = rhs.contentType;
	// TODO:

	return (*this);
}

void Response::fillHeader()
{
	protocolVersion = "HTTP/1.1";
	server = "webserv/1.0";

	contentType = "text/html";	// TODO:
	contentLength = body.length();
}

std::string Response::findFilePath(Request & request, Conf & conf)
{
	std::string path;
	struct stat infos;
	if(request.getPath()[request.getPath().length() - 1] != '/') // means file e.g. index.html
		path = conf.getRoot() + "/" + request.getPath();
	else
	{
		std::vector<std::string> indexes = conf.getIndex();
		for(std::vector<std::string>::iterator it = indexes.begin(); it != indexes.end(); it++)
		{
			path = conf.getRoot() + "/" + request.getPath() + *it;
			if (stat(path.c_str(), &infos) == -1)
				path = "";
			else
				break ;
		}
	}
	if (stat(path.c_str(), &infos) == -1)
	{
		statusCode = "404 Not Found";
		return ("");
	}
	else if (infos.st_mode & S_IFDIR || !(infos.st_mode & S_IRUSR))
	{
		statusCode = "403 Forbidden";
		return ("");
	}
	return (path);
}

void Response::fillBody(Request & request, Conf conf)
{

	statusCode = "200 OK";
	std::string path = findFilePath(request, conf);
	if (path.empty())
		return ;

	std::ifstream f(path.c_str());
	std::stringstream buf;
	buf << f.rdbuf();
	body = buf.str();
}