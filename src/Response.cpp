#include "Conf.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include <cstring>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <map>
#include <cstdio>

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
	this->body = rhs.body;
	// TODO:

	return (*this);
}

//================================================================================//

/*  LE PATH */
void Response::findFilePath(Request & request, Conf const& conf)
{
	struct stat infos;
	/* le path ne requiere pas de recherche d'index */
	if(request.getPath()[request.getPath().length() - 1] != '/')
		path = conf.getRoot() + "/" + request.getPath();
	
	/* itere sur les indexes pour trouver le bon path */
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

	/* gere les erreurs de stats du fichier demande */
	if (stat(path.c_str(), &infos) == -1)
	{
		statusCode = "404 Not Found";
		path = "";
		return;
	}
	else if (infos.st_mode & S_IFDIR || !(infos.st_mode & S_IRUSR))
	{
		statusCode = "403 Forbidden";
		path = "";
		return ;
	}
}

/* EN FONCTION DU STATUS CODE REMPLI LE BODY AVEC LA PAGE D'ERREUR CORRESPONDANTE, REMPLI LE HEADER ET RENVOI LA REPONSE FORMATEE EN STRING */
std::string Response::errorFillResponse(std::string code)
{
	//TODO : autres statuts d'erreur? 
	std::string path;
	std::stringstream ss;
	ss << "./error_pages/" << code;
	ss >> path;
	path += ".html";
	ss.str("");
	ss.clear();

	std::ifstream f(path.c_str());
	ss << f.rdbuf();
	body = ss.str();
	ss.str("");
	ss.clear();
	statusCode = code;
	fillHeader();

	return (format());
}

std::string Response::prepareResponse(Request & request, std::vector<Conf> & confs){
	if (!request.statusCode.empty())
		return (errorFillResponse(request.statusCode));
	fillBody(request, Utils::selectConf(confs, request.getServerName()));
	if (statusCode != "200 OK")
		return (errorFillResponse(statusCode));
	fillHeader();
	return (format());
}

std::string Response::matchingExtensionType(const std::string &extension)
{	
	std::ifstream mime_types("./conf/mime.types");
	std::stringstream ss;
	std::string type;
	std::string line;

	while(std::getline(mime_types, line))
	{
		if(line.find(extension) != std::string::npos)
		{
			ss << line;
			ss >> type;
			break ;
		}
	}
	if (type.empty())
		type = "application/octet-stream";
	return (type);
}

void Response::setContentType(){
	if (path.empty())
		contentType = "";

	std::string extension = path.substr(path.rfind(".") + 1);
	//std::cout << "extension: " << extension << std::endl;
	contentType = matchingExtensionType(extension);
	//std::cout << "content type found = " << contentType << std::endl;
}

void Response::fillHeader()
{
	protocolVersion = "HTTP/1.1";
	server = "webserv/1.0";

	setContentType();
	contentLength = body.length();
	//TODO autres champs de header ?
}

void Response::fillBody(Request & request, Conf const& conf)
{
	statusCode = "200 OK";
	if (request.getMethod() == "GET")
	{
		findFilePath(request, conf);
		if (path.empty())
			return ;
		std::ifstream f(path.c_str());
		std::stringstream buf;
		buf << f.rdbuf();
		body = buf.str();
	}
	else if (request.getMethod() == "DELETE")
	{
		struct stat infos;
		std::string path = conf.getRoot() + "/" + request.getPath();
		if (stat(path.c_str(), &infos) == -1)
		{
			statusCode = "404 Not Found";
			return ;
		}
		else if (!(infos.st_mode & S_IWUSR))
		{
			statusCode = "403 Forbidden";
			return ;
		}
		//else if (infos.st_mode & S_IFDIR)
		//else (409 Conflict) // dossier droit r mais pas droit r dans sous dossier
		//{}
		remove(path.c_str());
		statusCode = "204 No Content";
	}
	else if (request.getMethod() == "POST")
	{
	}
}

/* FORMATE L'OBJET REPONSE POUR CREER UNE STRING DE REPONSE AU CLIENT */
std::string Response::format() const {
	std::stringstream ss;

	ss << this->protocolVersion << ' ' << this->statusCode << '\n'; // status line
	ss << "Server: " << this->server << '\n';
	ss << "Content-Length: " << this->contentLength << '\n';
	ss << "Content-Type: " << this->contentType << '\n';
	ss << '\n';
	ss << this->body;
	return ss.str();
}
