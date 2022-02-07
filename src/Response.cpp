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

/*  SI NECESSAIRE RECHERCHE LE BON INDEX, REMPLI L'ATTRIBUT PATH ET VERIFIE LES ERREUR(existe, permission), PATH SET A EMPTY SI ERREURS */
void Response::constructPath(Request &request, Conf const &conf)
{
	struct stat infos;

	/* decoupe la queryString dans le path */
	queryString = request.getPath().substr(request.getPath().find("?") + 1);
	path = request.getPath().substr(0, request.getPath().find("?"));
	if (queryString == path)
		queryString = "";

	/* le path ne requiert pas de recherche d'index */
	if (path[path.length() - 1] != '/')
		path = conf.getRoot() + "/" + path;

	/* itere sur les indexes pour trouver le bon path */
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

	/* gere les erreurs de stats du fichier demande */
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

/* EN FONCTION DU STATUS CODE REMPLI LE BODY AVEC LA PAGE D'ERREUR CORRESPONDANTE, REMPLI LE HEADER ET RENVOI LA REPONSE FORMATEE EN STRING */
std::string Response::errorFillResponse(std::string code)
{
	// TODO: autres statuts d'erreur?
	std::string path;
	std::stringstream ss;
	// TODO:
	// si dans la conf j'ai error_page 403 PATH
	//	on utilise PATH
	// Sinon on calcule le path
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
	if (path.empty())
	{
		contentType = "";
		// TODO: return ?
	}

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

void Response::fillHeader()
{
	protocolVersion = "HTTP/1.1";
	server = "webserv";

	setContentType();
	contentLength = body.length();
	// TODO: autres champs de header ?
}
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

void Response::getFile(Request &request, Conf const &conf)
{
	/* Recupere le path */
	constructPath(request, conf);
	if (path.empty())
		return;
	//if (path.substr(path.substr(0, path.find("?")).rfind(".")) == ".php")
	if (path.substr(path.rfind(".")) == ".php")
	{
		launchCGI(request);
	}
	else
	{
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
		body = launchCGI(request);
		// TODO: POST
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

std::string Response::launchCGI(Request & request)
{
	std::stringstream ss;
	std::string response;
	int fds_in[2];
	int fds_out[2];

	if (pipe(fds_out) == -1)
	{
		std::cerr << "error: pipe syscall failed" << std::endl;
		exit(EXIT_FAILURE);
	}

	/* Rempli les arguments passes a exec */
	std::vector<const char *>args;

	std::cerr << "At begining of launchCGI, path is: " << path << std::endl;
	std::string tmp = path.substr(0, path.rfind("/")) + "../cgi/php-cgi";
//	std::cerr << "exec is " << (path.substr(0, path.rfind("/")) + "../cgi/php-cgi") << std::endl;
	args.push_back(tmp.c_str());	// TODO:
	args.push_back(path.c_str());
	args.push_back(NULL);
	/* Rempli les meta var */
	std::vector<const char *>env;

	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("REDIRECT_STATUS=true");

	std::string script_filename = (std::string("SCRIPT_FILENAME=") + path);
	env.push_back(script_filename.c_str());

	std::string request_method = (std::string("REQUEST_METHOD=") + request.getMethod());
	env.push_back(request_method.c_str());

	std::string query_string = (std::string("QUERY_STRING=") + queryString);
	env.push_back(query_string.c_str());

	std::string content_length;
	if (request.getMethod() == "POST")
	{
		env.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
		ss << "CONTENT_LENGTH=" << request.getBody().length();
		content_length = ss.str();
		if (ss.fail())
		{
			std::cerr << "Cannot convert body length" << std::endl;	// TODO:
		}
		std::cerr << "content len debug: " << content_length << '\n';
		env.push_back(content_length.c_str());

		/* Passer le body en entree standard du cgi */
		if (pipe(fds_in) == -1)
		{
			std::cerr << "error: pipe syscall failed" << std::endl;
			exit(EXIT_FAILURE);	// TODO: set status code
		}
		std::cerr << "body sent to stdin is : " << request.getBody().c_str() << std::endl;
		std::cerr << "body len to stdin is : " << request.getBody().length() << std::endl;
		if (write(fds_in[1], request.getBody().c_str(), request.getBody().length()) == -1) // TODO: != verif nombre de char ecrits ?
		{
			std::cerr << "error: write syscall failed" << std::endl;
			exit(EXIT_FAILURE);
		}
		if (close(fds_in[1]) == -1)
		{
			std::cerr << "error: close syscall failed" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	env.push_back(NULL);

	pid_t pid = fork();
	if (pid == -1)
	{
		std::cerr << "error: fork syscall failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	else if (pid == 0)
	{
		if (request.getMethod() == "POST" && dup2(fds_in[0], STDIN_FILENO) == -1)
		{
			std::cerr << "error: dup2 syscall failed" << std::endl;
			exit(EXIT_FAILURE);
		}
		if (close(fds_out[0]) == -1)
		{
			std::cerr << "error: close syscall failed" << std::endl;
			exit(EXIT_FAILURE);
		}
		if (request.getMethod() == "POST" && dup2(fds_out[1], STDOUT_FILENO) == -1)
		{
			std::cerr << "error: dup2 syscall failed" << std::endl;
			exit(EXIT_FAILURE);
		}
		if (chdir("./site/") == -1) // TODO: path ? //chdir(dans root/path - phpinfo.php);
		{
			std::cerr << "error: chdir syscall failed" << std::endl;
			exit(EXIT_FAILURE);
		}
		// root/dir/file
		std::cerr << "exec name: " << *args.begin() << std::endl;
		std::cerr << "args 1: " << *(args.begin() + 1) << std::endl;

		const char ** toto = &(*env.begin());
		while (toto && *toto)
		{
			std::cerr << "env = " << *toto << std::endl;
			toto++;
		}
		
		execve(*args.begin(), (char* const*)&(*args.begin()), (char* const*)&(*env.begin()));
		std::cerr << "error: execve syscall failed" << std::endl;
		exit(EXIT_FAILURE);
	}
	else
	{
		if (wait(NULL) == -1)
		{
			std::cerr << "error: wait syscall failed" << std::endl;
			exit(EXIT_FAILURE);
		}
		if (close(fds_out[1]) == -1)
		{
			std::cerr << "error: close syscall failed" << std::endl;
			exit(EXIT_FAILURE);
		}
		if (request.getMethod() == "POST" && close(fds_in[0]) == -1)
		{
			std::cerr << "error: close syscall failed" << std::endl;
			exit(EXIT_FAILURE);
		}

		/* Recupere le retour du cgi comme reponse : TODO a formatter */
		char buf2[BUF_SIZE + 1];
		ssize_t bytes_read;
		while ((bytes_read = read(fds_out[0], buf2, BUF_SIZE)) > 0)
		{
			buf2[bytes_read] = '\0';
			response += buf2;
		}
		if (bytes_read == -1)
		{
			std::cerr << "error: read syscall failed" << std::endl;
			exit(EXIT_FAILURE);
		}
		if (close(fds_out[0]) == -1)
		{
			std::cerr << "error: close syscall failed" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	return (response);
}