#include "Conf.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include "webserv.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>


Response::Response() {} // TODO:
Response::Response(Response const &rhs) { *this = rhs; }
Response::~Response() {}

Response &Response::operator=(Response const &rhs)
{
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

/*  SI NECESSAIRE RECHERCHE LE BON INDEX, REMPLI L'ATTRIBUT PATH ET VERIFIE LES ERREURS(existe, permission), PATH SET A EMPTY SI ERREURS */
void Response::constructPath(Request &request, Conf const &conf)
{
	struct stat infos;

	//-- decoupe la queryString dans le path
	queryString = request.getPath().substr(request.getPath().find("?") + 1);
	path = request.getPath().substr(0, request.getPath().find("?"));
	if (queryString == path)
		queryString = "";
	//-- Enleve le location path
	if (conf.locationPath.length())
		path = std::string("/") + path.substr(conf.locationPath.length());
	//-- le path ne requiert pas de recherche d'index
	if (path[path.length() - 1] != '/')
		path = conf.root + "/" + path;
	//-- itere sur les indexes pour trouver le bon path
	else 
	{
		for (std::vector<std::string>::const_iterator it = conf.index.begin(); it != conf.index.end(); it++)
		{
			path = conf.root + "/" + path + *it;
			if (stat(path.c_str(), &infos) == -1)
				path = "";
			else
				break;
		}
	}

	//-- gere les erreurs de stats du fichier demande
	if (stat(path.c_str(), &infos) == -1)
	{
		statusCode = NOT_FOUND;
		path = "";
		return;
	}
	else if (infos.st_mode & S_IFDIR || !(infos.st_mode & S_IRUSR))
	{
		statusCode = FORBIDDEN;
		path = "";
		return;
	}
}

/* PREPARE LA REPONSE POUR LES CAS D'ERREUR (REMPLI LE BODY AVEC LA PAGE D'ERREUR CORRESPONDANTE au status code,  rempli le header) LA RENVOIE FORMATEE EN STRING */
std::string Response::errorFillResponse(std::string code, Conf & conf)
{
	std::stringstream ss;
	int error_code;

	ss << code;
	ss >> error_code;
	if (!std::isdigit(code[0]) || ss.fail())
	{
		std::cerr << "Error: error code is not valid" << std::endl;	// TODO: error func
		exit(EXIT_FAILURE);
	}
	ss.str("");
	ss.clear();

	if (conf.errorPages.count(error_code))
	{
		path = conf.root + "/" + conf.errorPages[error_code];
	}
	else
	{
		ss << "./error_pages/" << error_code << ".html";
		ss >> path;
		ss.str("");
		ss.clear();
	}

	std::cerr << ">>>>>>PATH ERR = " << path << std::endl;

	std::ifstream f(path.c_str());
	ss << f.rdbuf();
	body = ss.str();
	statusCode = code;
	fillHeader();
	return (format());
}

/* EFFECTUE REDIRECTION : SET LOCATION DANS LE HEADER ET STATUS CODE */
void Response::redirected(int code, std::string const& url)
{
	if (code == 301)	// TODO: other redirect codes
		statusCode = MOVED_PERMANENTLY;
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

	std::cerr << "This is path in setContentType(): " << path << std::endl;
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
	if (type.empty())//TODO: Retirer le par defaut ?
		type = "application/octet-stream";
	contentType = type;
}

/* REMPLI LE HEADER */
void Response::fillHeader()
{
	std::cerr << "fillHeader() called" << std::endl;
	protocolVersion = "HTTP/1.1";
	server = "webserv";

	setContentType();
	contentLength = body.length();
	// TODO: autres champs de header ?
}

/* METHODE DELETE  SI LES DROITS LE PERMETTENT SUPPRIME LE FICHIER. SET LE STATUSCODE */
void Response::deleteFile(Request &request, Conf const &conf)
{
	struct stat infos;
	/* Recupere et verifie le path */
	std::string path = conf.root + "/" + request.getPath();
	if (stat(path.c_str(), &infos) == -2)
	{
		statusCode = NOT_FOUND;
		return;
	}
	else if (!(infos.st_mode & S_IWUSR))
	{
		statusCode = FORBIDDEN;
		return;
	}
	// else if (infos.st_mode & S_IFDIR)
	// else (408 Conflict) // dossier droit r mais pas droit r dans sous dossier
	//{}

	/* Supprime le fichier */
	remove(path.c_str());
	statusCode = NO_CONTENT;
}

/* REMPLI LE BODY SELON LA METHODE GET */
void Response::getFile(Request &request, Conf const &conf)
{
	/* Recupere le path */
	std::cerr << "getFile() called" << std::endl;
	constructPath(request, conf);
	if (path.empty())
		return;

	/* Le fichier demande est un CGI */
	//if (path.substr(path.substr(0, path.find("?")).rfind(".")) == ".php")
	if (path.substr(path.rfind(".")) == ".php")
	{
		std::cout << "cgi in get " << std::endl;
		launchCGI(request);
		std::cerr << "launchCGI() ended" << std::endl;
	}

	/* Le fichier n'est pas un CGI */
	else
	{
		std::cerr << "not cgi in GET" << std::endl;
		/* Ouvre le fichier et rempli le body */
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
	if (request.getMethod() == "GET") // localhost/phpinfo.php?key1=value1&key2=value2
	{
		getFile(request, conf);
	}
	else if (request.getMethod() == "DELETE")
	{
		deleteFile(request, conf);
	}
	else if (request.getMethod() == "POST")
	{
		constructPath(request, conf);
		launchCGI(request);
	}
}

/* FORMATE L'OBJET REPONSE POUR CREER UNE STRING DE REPONSE AU CLIENT */
std::string Response::format() const
{
	std::stringstream ss;

	ss << this->protocolVersion << ' ' << this->statusCode << '\n'; // status line
	ss << "Server: " << this->server << '\n';
	if (this->contentLength)
		ss << "Content-Length: " << this->contentLength << '\n';
	ss << "Content-Type: " << this->contentType << '\n';
	if (!this->location.empty())
		ss << "Location: " << this->location << '\n';
	ss << '\n';
	ss << this->body;
	return ss.str();
}

void Response::error(std::string const& status_code, std::string const& error)
{
	std::cerr << "error: (Response) " << error << std::endl;
	statusCode =  status_code;
}

void Response::launchCGI(Request & request)
{
	int fds_in[2];
	int fds_out[2];
	std::stringstream ss;

	if (pipe(fds_out) == -1) // OUVERTURE PIPE_OUT ====
		return (error(INTERNAL, "pipe syscall failed"));

	//-- Rempli les arguments passes a exec
	std::vector<const char *>args;
	args.push_back(CGI_PATH);	// TODO: guess cgi path if not set ? if already set in system ?
	args.push_back(path.c_str());
	args.push_back(NULL);
	
	//-- Rempli les meta var
	std::string script_filename = std::string("SCRIPT_FILENAME=") + path;
	std::string request_method = (std::string("REQUEST_METHOD=") + request.getMethod());
	std::string query_string = (std::string("QUERY_STRING=") + queryString);
	std::string content_length;

	std::vector<const char*> env;
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("REDIRECT_STATUS=true");
	env.push_back(script_filename.c_str());
	env.push_back(request_method.c_str());
	env.push_back(query_string.c_str());
	
	//-- meta var post
	if (request.getMethod() == "POST")
	{
		ss << "CONTENT_LENGTH=" << request.getBody().length();
		content_length = ss.str();
		if (ss.fail())
			return (error(INTERNAL, "cannot convert body length meta var"));
		env.push_back(content_length.c_str());
		env.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");

		//-- Passe le body en entree standard du cgi pour envoyer body post (redirige stdin)
		if (pipe(fds_in) == -1)// OUVERTURE PIPE IN ====
			return (error(INTERNAL, "pipe syscall failed"));
		if (write(fds_in[1], request.getBody().c_str(), request.getBody().length()) == -1)
			return (error(INTERNAL, "write syscall failed"));
		if (close(fds_in[1]) == -1)
			return (error(INTERNAL, "close syscall failed"));
	}
	env.push_back(NULL);

	for (size_t i = 0; i < args.size() - 1; i++)
		std::cout << "args: " << args[i] << '\n';
	std::cout << "-----hell\n";
	for (size_t i = 0; i < env.size() - 1; i++)
		std::cout << "env: " << env[i] << '\n';

	//-- Exec php-cgi : fork, redirige stdout pour recuperer la reponse cgi, execve cgi
	pid_t pid = fork();
	if (pid == -1)
		return (error(INTERNAL, "fork syscall failed"));
	else if (pid == 0)
	{
		if (request.getMethod() == "POST" && dup2(fds_in[0], STDIN_FILENO) == -1)
			return(error(INTERNAL, "dup2 syscall failed"));
		if (close(fds_out[0]) == -1)
			return (error(INTERNAL, "close syscall failed"));
		if (dup2(fds_out[1], STDOUT_FILENO) == -1)
			return (error(INTERNAL, "dup2 syscall failed"));
		if (chdir(path.substr(0, path.rfind("/") - 1).c_str()) == -1)
			return (error(NOT_FOUND, "chdir syscall failed"));
		std::cerr << "---before execve---\n";
		if (execve(*args.begin(), (char* const*)&(*args.begin()), (char* const*)&(*env.begin())) == -1)
			return (error(INTERNAL, "execve syscall failed"));
	}
	else
	{
		std::cout << "-----hell will wait\n";
		if (wait(NULL) == -1)
			return (error(INTERNAL, "wait syscall failed"));
		std::cout << "-----hell\n";
		if (close(fds_out[1]) == -1)
			return (error(INTERNAL, "close syscall failed"));
		if (request.getMethod() == "POST" && close(fds_in[0]) == -1)
			return (error(INTERNAL, "close syscall failed"));
		//-- Recupere le retour du cgi comme reponse
		std::cout << "-----hell\n";
		char buf2[BUF_SIZE + 1];
		ssize_t bytes_read;
		while ((bytes_read = read(fds_out[0], buf2, BUF_SIZE)) > 0)
		{
			buf2[bytes_read] = '\0';
			body += buf2;
		}
		if (bytes_read == -1)
			return (error(INTERNAL, "read syscall failed"));
		std::cout << "-----hell\n";
		if (close(fds_out[0]) == -1)
			return (error(INTERNAL, "close syscall failed"));
	}

	//-- Formate reponse cgi
	contentType = body.substr(body.find("Content-type: ") + 14, std::string::npos); //ligne contenttype
	contentType = contentType.substr(0, contentType.find("\n"));// valeur contenttype

	std::cout << "-----hell, content-type: " << contentType << '\n';
	ss.str("");
	ss.clear();
	ss << body;
	std::string line;
	while (std::getline(ss, line) && line != "" && line != "\r") 
		;
	body = "";
	while (std::getline(ss, line))
		body += line + '\n';

	/*
	std::cout << "--------\n";
	std::cout << "contenttype in CGI: " << contentType << '\n';
	std::cout << "body in CGI: " << body << '\n';
	std::cout << "--------\n";
	*/
}
