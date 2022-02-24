#include "Conf.hpp"
#include "CGI.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include "webserv.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <exception>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

Response::Response() {}
Response::Response(Response const &rhs) { *this = rhs; }
Response::~Response() {}

Response &Response::operator=(Response const &rhs)
{
	if (this == &rhs)
		return (*this);
	this->path = rhs.path;
	this->queryString = rhs.queryString;
	this->protocolVersion = rhs.protocolVersion;
	this->statusCode = rhs.statusCode;
	this->contentLength = rhs.contentLength;
	this->contentType = rhs.contentType;
	this->server = rhs.server;
	this->location = rhs.location;
	this->date = rhs.date;
	this->lastModified = rhs.lastModified;
	this->body = rhs.body;
	return (*this);
}

//================================================================================//

/*  SI NECESSAIRE RECHERCHE LE BON INDEX, REMPLI L'ATTRIBUT PATH ET VERIFIE LES ERREURS(existe, permission), PATH SET A EMPTY SI ERREURS */
void Response::constructPath(Request &request, Conf const &conf)
{
	struct stat infos;

	//-- Decoupe la queryString dans le path
	queryString = request.getPath().substr(request.getPath().find("?") + 1);
	path = request.getPath().substr(0, request.getPath().find("?"));
	if (queryString == path)
		queryString = "";
	//-- Le path ne requiert pas de recherche d'index
	if (path[path.length() - 1] != '/')
		path = conf.root + "/" + path;
	//-- Itere sur les indexes pour trouver le bon path
	else
	{
		for (std::vector<std::string>::const_iterator it = conf.index.begin(); it != conf.index.end(); it++)
		{
			if (stat((conf.root + "/" + path + *it).c_str(), &infos) != -1)
			{
				path = conf.root + "/" + path + *it;
				break;
			}
		}
	}
	bzero(&infos, sizeof(infos));
	//-- Gere AUTOINDEX
	if(!path.empty() && path[path.length() - 1] == '/' && conf.autoindex)
	{
		autoIndex(path, conf.root);
		path = "";
		return ;
	}
	//-- Gere les erreurs de stats du fichier demande
	//- N'existe pas
	else if (stat(path.c_str(), &infos) == -1 && !(infos.st_mode & S_IFDIR))
	{
		statusCode = NOT_FOUND;
		path = "";
		return;
	}
	//- Redirige les paths de dossiers ne se finissant pas par / ex : toto >> toto/
	else if (infos.st_mode & S_IFDIR	// si dossier
		&& request.getPath().find("?") == std::string::npos	// si pas de queryString
		&& request.getPath()[request.getPath().length() - 1] != '/')	// si ne se termine pas par un /
	{
		redirected(301, request.getPath() + "/");
	}
	//- Pas droits d'acces
	else if (infos.st_mode & S_IFDIR || !(infos.st_mode & S_IRUSR))
	{
		statusCode = FORBIDDEN;
		path = "";
		return;
	}
	//-- Recupere heure de derniere modification
	else
		this->lastModified = Utils::setTime(&infos.st_mtime);
}

/* PREPARE LA REPONSE POUR LES CAS D'ERREUR (REMPLI LE BODY AVEC LA PAGE D'ERREUR CORRESPONDANTE au status code, rempli le header)
LA RENVOIE FORMATEE EN STRING */
std::string Response::errorFillResponse(std::string code, Conf & conf)
{
	//-- Recupere code erreur et le verifie
	std::stringstream ss;
	int error_code;
	ss << code;
	ss >> error_code;
	ss.str("");
	ss.clear();

	//-- Choisit page erreur de la conf ou la page d'erreur par defaut
	if (conf.errorPages.count(error_code))
		path = conf.root + "/" + conf.errorPages[error_code];
	else
	{
		ss << "./error_pages/" << error_code << ".html";
		ss >> path;
		ss.str("");
		ss.clear();
	}

	//-- Rempli header et body
	std::ifstream f(path.c_str());
	ss << f.rdbuf();
	body = ss.str();
	statusCode = code;
	fillHeader();
	return (format());
}

void Response::autoIndex(std::string const& path, std::string const& root)
{
	std::stringstream bdy;
	bdy << "<html>\n<head><title>Index of ";
	bdy << path << "</title></head>\n";
	bdy << "<body>\n<h1>Index of " << path << "</h1><hr><pre>";
	bdy << "<a href=\"../\">../</a>\n";
	struct dirent *dir;
	DIR *d = opendir((root + path).c_str());
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if (std::string(dir->d_name) == "." || std::string(dir->d_name) == "..")
				continue ;
			bdy << "<a href=\"" << dir->d_name << "\">" << dir->d_name << "</a>\n";
		}
		closedir(d);
	}
	bdy << "</pre><hr></body>\n</html>";
	contentType = "text/html";
	body = bdy.str();
}

/* EFFECTUE REDIRECTION : SET LOCATION DANS LE HEADER ET STATUS CODE */
void Response::redirected(int code, std::string const& url)
{
	if (code == 300)
		statusCode = MULTIPLE_CHOICES;
	else if (code == 301)
		statusCode = MOVED_PERMANENTLY;
	else if (code == 302)
		statusCode = FOUND;
	else if (code == 303)
		statusCode = SEE_OTHER;
	else if (code == 304)
		statusCode = NOT_MODIFIED;
	else if (code == 305)
		statusCode = USE_PROXY;
	else if (code == 307)
		statusCode = TEMPORARY_REDIRECT;
	this->location = url;
}

/* REMPLI LA REPONSE ET LA RENVOIE FORMATEE EN STRING */
std::string Response::prepareResponse(Request &request, std::vector<Conf> &confs)
{
	/* Selectionne la conf pour la reponse */
	Conf & conf = Utils::selectConf(confs, request.getServerName(), request.getPath());
	/* Gere les erreurs presentes en amont */
	if (!request.statusCode.empty())
		return (errorFillResponse(request.statusCode, conf));
	/* Gere method not allowed */
	if (std::find(conf.allowedMethods.begin(), conf.allowedMethods.end(), request.getMethod()) == conf.allowedMethods.end())
		return (errorFillResponse(METHOD_NOT_ALLOWED, conf));
	/* Gere les redirection */
	if (conf.redirectCode)
		redirected(conf.redirectCode, conf.redirectURL);
	/* Ou rempli le body */
	else
	{
		fillBody(request, conf);
		if (statusCode != "200 OK")
			return (errorFillResponse(statusCode, conf));
	}
	/* Rempli le header */
	fillHeader();
	return (format());
}

/* SET ATTRIBUT CONTENT TYPE EN FONCTION EXTENSION DU PATH */
void Response::setContentType()
{
	if (!contentType.empty())
		return ;

	/* Recupere l'extension */
	std::string extension = path.substr(path.rfind(".") + 1);

	/* Compare et selectionne dans le fichier de ref */
	std::ifstream mime_types("./conf/mime.types");
	std::stringstream ss;
	std::string type;
	std::string line;

	while (std::getline(mime_types, line))
	{
		if (line.find(extension) != std::string::npos)
		{
			ss << line;
			ss >> type;
			break;
		}
	}
	if (type.empty())
		type = "application/octet-stream";
	contentType = type;
}

/* REMPLI LE HEADER */
void Response::fillHeader()
{
	protocolVersion = "HTTP/1.1";
	server = "webserv";

	time_t rawtime;
	time(&rawtime);
	this->date = Utils::setTime(&rawtime);

	setContentType();
	contentLength = body.length();
}

/* METHODE DELETE  SI LES DROITS LE PERMETTENT SUPPRIME LE FICHIER. SET LE STATUSCODE */
void Response::methodDelete(Request &request, Conf const &conf)
{
	struct stat infos;
	//-- Recupere et verifie le path
	std::string path = conf.root + "/" + request.getPath();
	std::cout << path << std::endl;
	if (stat(path.c_str(), &infos) == -1)
	{
		statusCode = NOT_FOUND;
		return;
	}
	else if (!(infos.st_mode & S_IWUSR))
	{
		statusCode = FORBIDDEN;
		return;
	}
	//-- Supprime le fichier
	remove(path.c_str());
	statusCode = NO_CONTENT;
}

/* REMPLI LE BODY SELON LA METHODE GET */
void Response::methodGet(Request &request, Conf const &conf)
{
	//-- Recupere le path
	constructPath(request, conf);
	if (path.empty())
		return;

	//-- Le fichier demande est un CGI
	if (path.rfind(".") != std::string::npos && (path.substr(path.rfind(".")) == ".php" || path.substr(path.rfind(".")) == ".py"))
	{
		CGI cgi;
		cgi.launchCGI(request, *this);
	}
	//-- Le fichier n'est pas un CGI
	else
	{
		//-- Ouvre le fichier et rempli le body
		std::ifstream ifs(path.c_str());
		std::stringstream buf;
		buf << ifs.rdbuf();
		body = buf.str();
	}
}

/* REMPLI L'ATTRIBUT BODY EN FONCTION DU PATH DEMANDE ET DE LA METHODE DEMANDEE */
void Response::fillBody(Request &request, Conf const &conf)
{
	statusCode = "200 OK";
	if (request.getMethod() == "GET")
	{
		methodGet(request, conf);
	}
	else if (request.getMethod() == "DELETE")
	{
		methodDelete(request, conf);
	}
	else if (request.getMethod() == "POST")
	{
		constructPath(request, conf);
		CGI cgi;
		cgi.launchCGI(request, *this);
	}
}

/* FORMATE L'OBJET REPONSE POUR CREER UNE STRING DE REPONSE AU CLIENT */
std::string Response::format() const
{
	std::stringstream ss;

	ss << this->protocolVersion << ' ' << this->statusCode << '\n';
	ss << "Server: " << this->server << '\n';
	if (this->contentLength)
		ss << "Content-Length: " << this->contentLength << '\n';
	ss << "Content-Type: " << this->contentType << '\n';
	if (!this->location.empty())
		ss << "Location: " << this->location << '\n';
	ss << "Date: " << this->date << '\n';
	if (!this->lastModified.empty())
		ss << "Last-Modified: " << this->lastModified << '\n';
	ss << '\n';
	ss << this->body;
	return ss.str();
}

void Response::error(std::string const& status_code, std::string const& error)
{
	std::cerr << "error: (Response) " << error << std::endl;
	statusCode = status_code;
}

std::string const& Response::getPath() const { return (this->path); }
std::string const& Response::getQueryString() const {return (this->queryString); }
