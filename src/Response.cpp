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

	//-- le path ne requiert pas de recherche d'index
	if (path[path.length() - 1] != '/')
		path = conf.getRoot() + "/" + path;
	//-- itere sur les indexes pour trouver le bon path
	else 
	{
		std::vector<std::string> indexes = conf.getIndex();
		for (std::vector<std::string>::iterator it = indexes.begin(); it != indexes.end(); it++)
		{
			path = conf.getRoot() + "/" + path + *it;
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
std::string Response::errorFillResponse(std::string code)
{
	std::cerr << "errorFillResponse() called" << std::endl;
	// TODO: autres statuts d'erreur?
	std::stringstream ss;
	// TODO:
	// si dans la conf j'ai error_page 403 PATH
	//	on utilise PATH
	// Sinon on calcule le path
	ss << "./error_pages/" << code;	// TODO: dynamic path
	ss >> path;
	path += ".html";
	ss.str("");
	ss.clear();

	std::ifstream f(path.c_str());
	ss << f.rdbuf();
	body = ss.str();
	statusCode = code;
	fillHeader();
	return (format());
}

/* REMPLI LA REPONSE ET LA RENVOIE FORMATEE EN STRING */
std::string Response::prepareResponse(Request &request, std::vector<Conf> &confs)
{
	/* Gere les erreurs presentes en amont */
	if (!request.statusCode.empty())
		return (errorFillResponse(request.statusCode));
	/* Rempli le body */
	fillBody(request, Utils::selectConf(confs, request.getServerName()));
	if (statusCode != "200 OK")
		return (errorFillResponse(statusCode));
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
	std::string extension = path.substr(path.rfind(".") + 1);

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
	std::string path = conf.getRoot() + "/" + request.getPath();
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
	ss << "Content-Length: " << this->contentLength << '\n';
	ss << "Content-Type: " << this->contentType << '\n';
	ss << '\n';
	ss << this->body;
	return ss.str();
}

int error(std::string error) // TODO : gestion erreur Launch cgi
{
	std::cerr << error << std::endl;
	return(EXIT_FAILURE);
}

void Response::launchCGI(Request & request)
{
	int fds_in[2];
	int fds_out[2];
	std::stringstream ss;

	if (pipe(fds_out) == -1) // OUVERTURE PIPE_OUT ====
		exit(error("error: pipe syscall failed"));

	//* Rempli les arguments passes a exec
	std::vector<const char *>args;
	std::string tmp = path.substr(0, path.rfind("/")) + "../cgi/php-cgi";
	args.push_back(tmp.c_str());	// TODO:
	args.push_back(path.c_str());
	args.push_back(NULL);
	
	//* Rempli les meta var
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

	//* meta var post
	if (request.getMethod() == "POST")
	{
		ss << "CONTENT_LENGTH=" << request.getBody().length();
		content_length = ss.str();
		if (ss.fail())
			exit(error("Cannot convert body length meta var"));
		env.push_back(content_length.c_str());
		env.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");

		//* Passe le body en entree standard du cgi 
		if (pipe(fds_in) == -1)// OUVERTURE PIPE IN ====
			exit(error("error: pipe syscall failed"));	// TODO: set status code
		if (write(fds_in[1], request.getBody().c_str(), request.getBody().length()) == -1) // TODO: != verif nombre de char ecrits ?
			exit(error("error: write syscall failed"));
		if (close(fds_in[1]) == -1)
			exit(error("error: close syscall failed"));
	}
	env.push_back(NULL);

	pid_t pid = fork();
	if (pid == -1)
		exit(error("error: fork syscall failed"));
	else if (pid == 0)
	{
		if (request.getMethod() == "POST" && dup2(fds_in[0], STDIN_FILENO) == -1)
			exit(error("error: dup2 syscall failed"));
		if (close(fds_out[0]) == -1)
			exit(error("error: close syscall failed"));
		if (dup2(fds_out[1], STDOUT_FILENO) == -1)
			exit(error("error: dup2 syscall failed"));
		if (chdir("./site/") == -1) // TODO: path ? //chdir(root/path - phpinfo.php);
			exit(error("error: chdir syscall failed"));
		//* Exec php-cgi
		if (execve(*args.begin(), (char* const*)&(*args.begin()), (char* const*)&(*env.begin())) == -1)
			exit(error("error: execve syscall failed"));
	}
	else
	{
		if (wait(NULL) == -1)
			exit(error("error: wait syscall failed"));
		if (close(fds_out[1]) == -1)
			exit(error("error: close syscall failed"));
		if (request.getMethod() == "POST" && close(fds_in[0]) == -1)
			exit(error("error: close syscall failed"));
		//* Recupere le retour du cgi comme reponse
		char buf2[BUF_SIZE + 1];
		ssize_t bytes_read;
		while ((bytes_read = read(fds_out[0], buf2, BUF_SIZE)) > 0)
		{
			buf2[bytes_read] = '\0';
			body += buf2;
		}
		if (bytes_read == -1)
			exit(error("error: read syscall failed"));
		if (close(fds_out[0]) == -1)
			exit(error("error: close syscall failed"));
	}

	//* Formate reponse cgi
	contentType = body.substr(body.find("Content-type: ") + 14, std::string::npos); //ligne contenttype
	contentType = contentType.substr(0, contentType.find("\n"));// valeur contenttype

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
