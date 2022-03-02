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

Response::Response() : sendLength(0), contentLength(0) { this->sendBuf = std::string(4096, '\0'); }
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
	this->tmpFilename = rhs.tmpFilename;
	this->sendBuf = rhs.sendBuf;
	this->sendLength = rhs.sendLength;
	return (*this);
}

//================================================================================//

/* METHODE DELETE SI LES DROITS LE PERMETTENT SUPPRIME LE FICHIER. SET LE STATUSCODE */
void Response::methodDelete(Request &request, Conf const &conf)
{
	struct stat infos;
	//-- Recupere et verifie le path
	std::string path = conf.root + "/" + request.getPath();
	if (stat(path.c_str(), &infos) == -1)
		statusCode = NOT_FOUND;
	else if (!(infos.st_mode & S_IWUSR))
		statusCode = FORBIDDEN;
	//-- Supprime le fichier
	else
	{
		remove(path.c_str());
		statusCode = NO_CONTENT;
	}
}

/* PAGE D'AUTO INDEX REALISEE DANS UN tmpFile */
void Response::autoIndex(std::string const& path, std::string const& root)
{
	createTMPFile();
	this->tmpFile << "<html>";
	this->tmpFile << "<head><title>Index of " << path.c_str() << "</title></head>";
	this->tmpFile << "<body>";
	this->tmpFile << "<h1>Index of " << path.c_str() << "</h1>";
	this->tmpFile << "<hr>";
	this->tmpFile << "<pre>";
	this->tmpFile << "<a href=\"../\">../</a>";
	struct dirent *dir;
	DIR *d = opendir((root + path).c_str());
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			if (std::string(dir->d_name) == "." || std::string(dir->d_name) == "..")
				continue ;
			this->tmpFile << "<a href=\"" << dir->d_name << "\">" << dir->d_name << "</a><br>";
		}
		closedir(d);
	}
	this->tmpFile << "</pre>";
	this->tmpFile << "<hr>";
	this->tmpFile << "</body>";
	this->tmpFile << "</html>";
	this->tmpFile.flush();
	contentType = "text/html";
	this->path = this->tmpFilename;
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
	this->path = "";
	this->contentLength = 0;
}

void Response::createTMPFile()
{
	tmpFilename = "/tmp/webserv_XXXXXX";
	int fd = mkstemp(&(*tmpFilename.begin()));
	if (fd != -1)
		close(fd);
	this->tmpFile.open(tmpFilename.c_str(), std::fstream::out | std::fstream::in | std::fstream::binary | std::fstream::trunc);
}

/* SI NECESSAIRE RECHERCHE LE BON INDEX, REMPLI L'ATTRIBUT PATH ET VERIFIE LES ERREURS(existe, permission), PATH SET A EMPTY SI ERREURS */
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

int Response::emptyHeadersInFile()
{
	std::vector<std::string> fields;
	fields.push_back("host");
	fields.push_back("transfer-encoding");
	fields.push_back("accept");
	fields.push_back("accept-encoding");
	fields.push_back("accept-language");
	fields.push_back("cache-control");
	fields.push_back("connection");
	fields.push_back("expect");
	fields.push_back("if-modified-since");
	fields.push_back("origin");
	fields.push_back("postman-token");
	fields.push_back("pragma");
	fields.push_back("referer");
	fields.push_back("sec-fetch-dest");
	fields.push_back("sec-fetch-mode");
	fields.push_back("sec-fetch-site");
	fields.push_back("sec-fetch-user");
	fields.push_back("upgrade-insecure-requests");
	fields.push_back("user-agent");
	fields.push_back("x-powered-by");
	fields.push_back("status");
	// Response specific fields
	fields.push_back("location");
	fields.push_back("date");
	fields.push_back("last-modified");
	fields.push_back("server");
	fields.push_back("content-length");
	fields.push_back("content-type");

	std::string line;
	int header_size = 0;
	while (std::getline(this->tmpFile, line) && line != "\r")
	{
		std::vector<std::string>::iterator it;
		for (it = fields.begin(); it != fields.end(); ++it)
		{
			if (Utils::tolowerstr(line).find(*it) == 0)
			{
				if (*it == "location")
					location = "";
				else if (*it == "date")
					date = "";
				else if (*it == "last-modified")
					lastModified = "";
				else if (*it == "content-type")
					contentType = "";
				else if (*it == "content-length")
					contentLength = -1;
				header_size += line.length();
				break ;
			}
		}
		if (it == fields.end())
		{
			return (0);
		}
	}
	if (line == "\r")
		return (header_size);
	return (0);
}

void Response::error(std::string const& status_code, std::string const& error)
{
	std::cerr << "error: (Response) " << error << std::endl;
	statusCode = status_code;
}

std::string const& Response::getPath() const { return (this->path); }
std::string const& Response::getQueryString() const {return (this->queryString); }

/* SET ATTRIBUT CONTENT TYPE EN FONCTION EXTENSION DU PATH */
void Response::setContentType()
{
	if (!contentType.empty())
		return ;

	//-- Recupere l'extension
	std::string extension = path.substr(path.rfind(".") + 1);

	//-- Compare et selectionne dans le fichier de ref
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

void Response::setContentLength(int headerSize)
{
	if (this->contentLength == -1 || this->path.empty())
	{
		this->contentLength = 0;
		return ;
	}
	struct stat infos;
	stat(this->path.c_str(), &infos);
	this->contentLength = infos.st_size - headerSize;
}

void Response::firstFillSendBuf(int header_len)
{
	this->bodyStream.open(this->path);
	this->sendLength = header_len;
	this->bodyStream.read(&this->sendBuf[header_len], BUF_SIZE - header_len);
	this->sendLength += this->bodyStream.gcount();
}

/* PREPARE LE HEADER ET LA REPONSE ET L'ENVOIE AU CLIENT BUFFURISEE */
void Response::prepareResponse(Request &request, std::vector<Conf> &confs)
{
	//-- Le header est deja envoye, on envoie le body bufferisé
	if (bodyStream.is_open())
	{
		this->bodyStream.read(&this->sendBuf[0], BUF_SIZE);
		this->sendLength = this->bodyStream.gcount();
		return ;
	}
	Conf & conf = Utils::selectConf(confs, request.getServerName(), request.getPath());
	//-- Gere les erreurs presentes en amont
	if (!request.statusCode.empty())
		fillErrorBody(request.statusCode, conf);
	//-- Gere les redirections
	else if (conf.redirectCode)
		redirected(conf.redirectCode, conf.redirectURL);
	//-- Prepare le path du fichier a transmettre
	else
		prepareBody(request, conf);
	//-- Rempli le sendbuf avec le header
	int header_len = fillHeader();
	//-- Ouvre le bodystream, rempli la fin du premier sendbuf avec le debut du body
	if (path.empty())
		return ;
	firstFillSendBuf(header_len);
}

/* Prepare en fonction de la methode le path du fichier a transmettre (GET ou result cgi, autoindex, erreur) ou a supprimer */
void Response::prepareBody(Request & request, Conf &conf)
{
	statusCode = "200 OK";
	if (request.getMethod() == "POST" && path.rfind(".") != std::string::npos && conf.cgi.count(path.substr(path.rfind("."))) == 0)	// POST && pas .py ni .php
		fillErrorBody(METHOD_NOT_ALLOWED, conf);
	else if (request.getMethod() == "GET" || request.getMethod() == "POST")
	{
		constructPath(request, conf);
		if (statusCode != "200 OK")
			fillErrorBody(statusCode, conf);
	}
	else if (request.getMethod() == "DELETE")
	{
		methodDelete(request, conf);
		this->path = "";
		if (statusCode != "200 OK")
			fillErrorBody(statusCode, conf);
	}
	if (path.rfind(".") != std::string::npos && conf.cgi.count(path.substr(path.rfind("."))) > 0) //-- extension .php ou .py
	{
		createTMPFile();
		CGI cgi;
		cgi.launchCGI(request, *this, conf);
		this->path = this->tmpFilename;
	}
}

/* REMPLI LE HEADER */
int Response::fillHeader()
{
	//-- Set les variables headers
	protocolVersion = "HTTP/1.1";
	server = "webserv";

	time_t rawtime;
	time(&rawtime);
	this->date = Utils::setTime(&rawtime);
	setContentType();

	//-- Vide les headers deja presents dans le fichier
	int header_size = emptyHeadersInFile();
	setContentLength(header_size);

	//-- Rempli le sendBuf avec les variables des headers
	std::string buf;
	buf += protocolVersion + ' ' + statusCode + "\r\n";
	if (!this->server.empty())
		buf += "Server: " + this->server + "\r\n";
	if (this->contentLength)
	{
		std::stringstream ss;
		ss << this->contentLength;
		buf += "Content-Length: " + ss.str() + "\r\n";
	}
	if (!this->contentType.empty())
		buf += "Content-Type: " + this->contentType + "\r\n";
	if (!this->location.empty())
		buf += "Location: " + this->location + "\r\n";
	if (!this->date.empty())
		buf += "Date: " + this->date + "\r\n";
	if (!this->lastModified.empty())
		buf += "Last-Modified: " + this->lastModified + "\r\n";
	if (!header_size)
		buf += "\r\n";
	this->sendBuf.replace(0, buf.length(), buf);
	return (buf.length());
}

/* SELECTIONNE LA PAGE DE BODY d'ERREUR CORRESPONDANT AU CODE PASSE EN PARAMETRE */
void Response::fillErrorBody(std::string code, Conf & conf)
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
	statusCode = code;
}
