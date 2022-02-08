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

std::string launchCGI(); // TODO: remove

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
	std::cerr << "constructPath() called" << std::endl;
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
		//body = launchCGI(request);
		body = launchCGI();
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
		//body = launchCGI(request);// TODO: disable comment
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

int error(std::string error)
{
	std::cerr << error << std::endl;
	return(EXIT_FAILURE);
}
/*
std::string Response::launchCGI(Request & request)
{
	std::stringstream ss;
	std::string budy;
	//int fds_in[2];
	(void)request;	// TODO: remove
	int fds_out[2];

	if (pipe(fds_out) == -1) // OUVERTURE PIPE_OUT ====
		exit(error("error: pipe syscall failed"));

// Rempli les arguments passes a exec
	std::vector<const char *>args;
	std::cerr << "At begining of launchCGI, path is: " << path << std::endl;
	std::string tmp = path.substr(0, path.rfind("/")) + "../cgi/php-cgi";
	//std::cerr << "exec is " << (path.substr(0, path.rfind("/")) + "../cgi/php-cgi") << std::endl;
//	args.push_back(tmp.c_str());	// TODO:
//	args.push_back(path.c_str());
	args.push_back("/home/aurelie/Documents/webserv/cgi/php-cgi");
	args.push_back("/home/aurelie/Documents/webserv/site/phpinfo.php");
	args.push_back(NULL);
	args.push_back("../cgi/php-cgi");
	args.push_back("./phpinfo.php");
	args.push_back(NULL);
	
// Rempli les meta var
	std::vector<const char*> env;
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("REDIRECT_STATUS=true");
	env.push_back("SCRIPT_FILENAME=./phpinfo.php");
	env.push_back("REQUEST_METHOD=GET");
	env.push_back("QUERY_STRING=toto=6&send=send");
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("REDIRECT_STATUS=true");
	env.push_back("SCRIPT_FILENAME=/home/aurelie/Documents/webserv/site/phpinfo.php");

	//std::string script_filename = (std::string("SCRIPT_FILENAME=/home/aurelie/Documents/webserv/site///phpinfo.php");
//	std::string script_filename = (std::string("SCRIPT_FILENAME=") + path);
	//env.push_back(script_filename.c_str());

	//std::string request_method = (std::string("REQUEST_METHOD=GET"));
//	std::string request_method = (std::string("REQUEST_METHOD=") + request.getMethod());
	//env.push_back(request_method.c_str());

	//std::string query_string = (std::string("QUERY_STRING=") + queryString);
	//std::string query_string = (std::string("QUERY_STRING=\"name=toto\""));
	//env.push_back(query_string.c_str());

	env.push_back("REQUEST_METHOD=GET");
	env.push_back("QUERY_STRING=toto=6&send=send");
//	env.push_back(NULL);
	std::string content_length;
// meta var post
POST
	if (request.getMethod() == "POST")
	{
		env.push_back("CONTENT_TYPE=application/x-www-form-urlencoded");
		ss << "CONTENT_LENGTH=" << request.getBody().length();
		content_length = ss.str();
		if (ss.fail())
			std::cerr << "Cannot convert body length" << std::endl;	// TODO:
		std::cerr << "content len debug: " << content_length << '\n';
		env.push_back(content_length.c_str());

// Passe le body en entree standard du cgi 
		if (pipe(fds_in) == -1)// OUVERTURE PIPE IN ====
			exit(error("error: pipe syscall failed"));	// TODO: set status code

		std::cerr << "body sent to stdin is : " << request.getBody().c_str() << std::endl;
		std::cerr << "body len to stdin is : " << request.getBody().length() << std::endl;

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
		POST
		if (request.getMethod() == "POST" && dup2(fds_in[0], STDIN_FILENO) == -1)
			exit(error("error: dup2 syscall failed"));
		if (close(fds_out[0]) == -1)
			exit(error("error: close syscall failed"));
		if (dup2(fds_out[1], STDOUT_FILENO) == -1)
			exit(error("error: dup2 syscall failed"));
		if (chdir("./site/") == -1) // TODO: path ? //chdir(root/path - phpinfo.php);
			exit(error("error: chdir syscall failed"));

		const char ** toto = &(*args.begin());
		while (toto && *toto)
		{
			std::cerr << "args = " << *toto << std::endl;
			toto++;
		}
		std::cerr << "==============" << std::endl;
		toto = &(*env.begin());
		while (toto && *toto)
		{
			std::cerr << "env = " << *toto << std::endl;
			toto++;
		}

// Exec php-cgi  
		std::cerr << "execve(" << *args.begin() << ", " << (*args.begin()) << ", " << (*env.begin()) << ")" << std::endl;
		if (execve(*args.begin(), (char* const*)&(*args.begin()), (char* const*)&(*env.begin())) == -1)
			exit(error("error: execve syscall failed"));
	}
	else
	{
		if (wait(NULL) == -1)
			exit(error("error: wait syscall failed"));
		std::cerr << "execve ended" << std::endl;
		
		if (close(fds_out[1]) == -1)
			exit(error("error: close syscall failed"));
		POST
		if (request.getMethod() == "POST" && close(fds_in[0]) == -1)
			exit(error("error: close syscall failed"));

// Recupere le retour du cgi comme reponse : TODO a formatter

		char buf2[BUF_SIZE + 1];
		ssize_t bytes_read;
		while ((bytes_read = read(fds_out[0], buf2, BUF_SIZE)) > 0)
		{
			buf2[bytes_read] = '\0';
			budy += buf2;
		}
		if (bytes_read == -1)
			exit(error("error: read syscall failed"));
		budy = "Content-type: text/html; charset=UTF-8\r\n\r\n<h1>call to phpinfo.php\r\n</h1><pre>GET = array(2) {\r\n[\"toto\"]=>\r\nstring(1) \"6\"\r\n[\"send\"]=>\r\nstring(4) \"send\"\r\n}\r\nPOST = array(0) {\r\n}\r\n</pre><p>0 Hello World!<br />\r\n1 Hello World!<br />\r\n2 Hello World!<br />\r\n3 Hello World!<br />\r\n4 Hello World!<br />\r\n</p>";
		if (close(fds_out[0]) == -1)
			exit(error("error: close syscall failed"));
	}
	contentType = budy.substr(budy.find(": ") + 2, std::string::npos);
	contentType = contentType.substr(0, contentType.find("\n"));
	budy = budy.substr(budy.find("\n") + 1);	// enlever la ligne content-type
	budy = budy.substr(budy.find("\n") + 1);	// enlever la ligne vide
	std::cout << "--------\n";
	std::cout << "contenttype in CGI: " << contentType << '\n';
	std::cout << "body in CGI: " << budy << '\n';
	std::cout << "--------\n";
	return (budy);
}
*/

std::string launchCGI()
{
//	(void)request;
	std::cout << "debut launchCGI cout" << std::endl;
	std::string path("/home/aurelie/Documents/webserv/site///phpinfo.php");

	std::cout << "to hello: " << std::endl;
	std::stringstream ss;
	std::cout << "to hello1: " << std::endl;
	std::string budy;
	std::cout << "to hello2: " << std::endl;
	int fds_out[2];

	std::cout << "to hell: " << std::endl;
	if (pipe(fds_out) == -1) // OUVERTURE PIPE_OUT ====
		exit(error("error: pipe syscall failed"));

/* Rempli les arguments passes a exec */
	std::vector<const char *>args;
	std::cout << "At begining of launchCGI, path is: " << path << std::endl;
	std::string tmp = path.substr(0, path.rfind("/")) + "../cgi/php-cgi";
	//std::cout << "exec is " << (path.substr(0, path.rfind("/")) + "../cgi/php-cgi") << std::endl;
	//args.push_back(tmp.c_str());	// TODO:
	//args.push_back(path.c_str());
	std::cout << "to hello3: " << std::endl;
	args.push_back("../cgi/php-cgi");
	args.push_back("./phpinfo.php");
	args.push_back(NULL);
	
	std::cout << "to hello4: " << std::endl;
/* Rempli les meta var */
	std::vector<const char *>env;
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("REDIRECT_STATUS=true");
	env.push_back("SCRIPT_FILENAME=./phpinfo.php");
	std::cout << "to hello5: " << std::endl;

	//std::string script_filename = (std::string("SCRIPT_FILENAME=/home/aurelie/Documents/webserv/site///phpinfo.php");
//	std::string script_filename = (std::string("SCRIPT_FILENAME=") + path);
	//env.push_back(script_filename.c_str());

	//std::string request_method = (std::string("REQUEST_METHOD=GET"));
//	std::string request_method = (std::string("REQUEST_METHOD=") + request.getMethod());
	//env.push_back(request_method.c_str());

	//std::string query_string = (std::string("QUERY_STRING=") + queryString);
	//std::string query_string = (std::string("QUERY_STRING=\"name=toto\""));
	//env.push_back(query_string.c_str());

	env.push_back("REQUEST_METHOD=GET");
	env.push_back("QUERY_STRING=toto=6&send=send");
	std::string content_length;
/* meta var post */
	
	env.push_back(NULL);

	std::cout << "to hello6: " << std::endl;
	pid_t pid = fork();
	if (pid == -1)
		exit(error("error: fork syscall failed"));
	else if (pid == 0)
	{

		std::cout << "to hello7: " << std::endl;
		if (close(fds_out[0]) == -1)
			exit(error("error: close syscall failed"));
//		if (dup2(fds_out[1], STDOUT_FILENO) == -1)
//			exit(error("error: dup2 syscall failed"));
		if (chdir("./site/") == -1) // TODO: path ? //chdir(root/path - phpinfo.php);
			exit(error("error: chdir syscall failed"));
		// root/dir/file

		std::cout << "==============" << std::endl;
		const char ** toto = &(*args.begin());
		while (toto && *toto)
		{
			std::cout << "args = " << *toto << std::endl;
			toto++;
		}
		std::cout << "==============" << std::endl;
		toto = &(*env.begin());
		while (toto && *toto)
		{
			std::cout << "env = " << *toto << std::endl;
			toto++;
		}
		std::cout << "==============" << std::endl;
		char **cmds;
		cmds = (char**)malloc(sizeof(char*) * 3);
		*(cmds + 2) = 0;
		char exec[] = "/home/aurelie/Documents/webserv/cgi/php-cgi";
		char file[] = "/home/aurelie/Documents/webserv/site/phpinfo.php";
		//for (int i = 0; i < 3; i++) cmds[i] = calloc(1,1);
		cmds[0] = strdup(exec);
		cmds[1] = strdup(file);
		char **genvp;
		char env0[] = "REDIRECT_STATUS=true";
		char env1[] = "REQUEST_METHOD=GET";
		char env2[] = 	"QUERY_STRING=prenom=antoine&nom=gautier";
		char env3[] = 	"SCRIPT_FILENAME=/home/aurelie/Documents/webserv/site/phpinfo.php";
		char env4[] = 	"GATEWAY_INTERFACE=CGI/1.1";
		genvp = (char**)malloc(sizeof(char*) * 6);
		*(genvp + 5) = 0;
		genvp[0] = strdup(env0);
		genvp[1] = strdup(env1);
		genvp[2] = strdup(env2);
		genvp[3] = strdup(env3);
		genvp[4] = strdup(env4);
		//for (int i = 0; i < 6; i++) genvp[i] = calloc(1,1);
/*
*/
/* Exec php-cgi */ 
		//std::cout << "execve(" << *args.begin() << ", " << (*(args.begin() + 1)) << ", " << (*env.begin()) << ")" << std::endl;

		const char* argt[] = {
			"/home/aurelie/Documents/webserv/cgi/php-cgi",
			"/home/aurelie/Documents/webserv/site/phpinfo.php",
			NULL
		};
		/*
		const char * envt[] = {
			"GATEWAY_INTERFACE=CGI/1.1",
			"REDIRECT_STATUS=true",
			"SCRIPT_FILENAME=/home/aurelie/Documents/webserv/site///phpinfo.php",
			"REQUEST_METHOD=GET",
			"QUERY_STRING=toto=6&send=send",
			NULL,
		};
		const char* envt[] = {
			"REDIRECT_STATUS=true",
			"REQUEST_METHOD=GET",
			"QUERY_STRING=prenom=antoine&nom=gautier",
			"SCRIPT_FILENAME=/home/aurelie/Documents/webserv/site/phpinfo.php",
			"GATEWAY_INTERFACE=CGI/1.1",
			NULL
		};
		*/

//		char * const* parg = (char * const*)calloc(sizeof(const char *), 3);
//		parg[0] = "/home/aurelie/Documents/webserv/cgi/php-cgi";
//		parg[1] = "/home/aurelie/Documents/webserv/site/phpinfo.php";
//		parg[2] = NULL;
//
//		char * const* penv = (char * const *)calloc(sizeof(const char *), 7);
//		penv[0] = "REDIRECT_STATUS=true";
//		penv[1] = "REQUEST_METHOD=GET";
//		penv[2] = "QUERY_STRING=prenom=antoine&nom=gautier";
//		penv[3] = "SCRIPT_FILENAME=/home/aurelie/Documents/webserv/site/phpinfo.php";
//		penv[4] = "GATEWAY_INTERFACE=CGI/1.1";
//		penv[5] = NULL;

		std::cout << "execve(" << argt[0] << ", " << argt[0] << ", " << argt[1] << ", " << (*env.begin()) << ")" << std::endl;
//		(void)envt;
//		if (dup2(fds_out[1], STDOUT_FILENO) == -1)
	//		exit(error("error: dup2 syscall failed"));
        if (execve(cmds[0], cmds, genvp) == -1)
        //if (execve(argt[0], (char * const*)argt, (char * const*)envt) == -1)
        //if (execve(argt[0], (char* const*)argt, (char* const*)envt) == -1)
       // if (execve(*args.begin(), (char* const*)&(*args.begin()), (char* const*)&(*env.begin())) == -1)
			exit(error("error: execve syscall failed"));
	}
	else
	{
		if (wait(NULL) == -1)
			exit(error("error: wait syscall failed"));
		std::cout << "execve ended" << std::endl;
		
		if (close(fds_out[1]) == -1)
			exit(error("error: close syscall failed"));

/* Recupere le retour du cgi comme reponse : TODO a formatter */
		char buf2[BUF_SIZE + 1];
		ssize_t bytes_read;
		while ((bytes_read = read(fds_out[0], buf2, BUF_SIZE)) > 0)
		{
			buf2[bytes_read] = '\0';
			budy += buf2;
		}
		if (bytes_read == -1)
			exit(error("error: read syscall failed"));
  /*
		budy = "Content-type: text/html; charset=UTF-8\r\n\r\n<h1>call to phpinfo.php\r\n</h1><pre>GET = array(2) {\r\n[\"toto\"]=>\r\nstring(1) \"6\"\r\n[\"send\"]=>\r\nstring(4) \"send\"\r\n}\r\nPOST = array(0) {\r\n}\r\n</pre><p>0 Hello World!<br />\r\n1 Hello World!<br />\r\n2 Hello World!<br />\r\n3 Hello World!<br />\r\n4 Hello World!<br />\r\n</p>";
		*/
		if (close(fds_out[0]) == -1)
			exit(error("error: close syscall failed"));

		std::cout << "================" << std::endl;
		std::cout << "budy is |\n" << budy << "|\n";
		std::cout << "================" << std::endl;

		/*
		std::string contentType = budy.substr(budy.find(": ") + 2, std::string::npos);
		contentType = contentType.substr(0, contentType.find("\n"));
		budy = budy.substr(budy.find("\n") + 1);	// enlever la ligne content-type
		budy = budy.substr(budy.find("\n") + 1);	// enlever la ligne vide

		std::cout << "--------\n";
		std::cout << "contenttype in CGI: " << contentType << '\n';
		std::cout << "body in CGI: " << budy << '\n';
		std::cout << "--------\n";
		*/

		std::cout << "RETURN IS " << budy << std::endl;
	}
	return (budy);
}