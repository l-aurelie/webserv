#include "Conf.hpp"
#include "CGI.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "webserv.hpp"

#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <iostream>

CGI::CGI()
{
	this->fds_out[0] = -1;
	this->fds_out[1] = -1;
}
CGI::CGI(CGI const& rhs) { *this = rhs; }
CGI::~CGI() {}

CGI &CGI::operator=(CGI const& rhs)
{
	if (this == &rhs)
		return (*this);
	this->scriptFilename = rhs.scriptFilename;
	this->requestMethod = rhs.requestMethod;
	this->queryString = rhs.queryString;
	this->contentLength = rhs.contentLength;
	this->contentType = rhs.contentType;
	this->uploadDir = rhs.uploadDir;

	this->fds_out[0] = rhs.fds_out[0];
	this->fds_out[1] = rhs.fds_out[1];
	this->args = rhs.args;
	this->env = rhs.env;
	return (*this);
}

//================================================================//

void CGI::initCGIArgs(Response & response, Conf & conf)
{
	std::string const& path = response.getPath();
	args.push_back(conf.cgi[path.substr(path.rfind("."))].c_str());
	/* Verifie que la path du cgi est correct */
	struct stat infos;
	if (stat(conf.cgi[path.substr(path.rfind("."))].c_str(), &infos) == -1)
	{
		args = std::vector< const char *>();
		return (response.error(INTERNAL, "cgi bin not found"));
	}
	if (!(infos.st_mode & S_IXUSR))
	{
		args = std::vector< const char *>();
		return (response.error(INTERNAL, "cannot execute cgi bin"));
	}
	args.push_back(response.getPath().c_str());
	args.push_back(NULL);
}

/* INITIALISE LES ARGUMENTS ET L'ENVIRONNEMENT DU CGI */
void CGI::initCGIEnv(Request const& request, Response & response, Conf const& conf)
{
	scriptFilename = std::string("SCRIPT_FILENAME=") + response.getPath();
	requestMethod = std::string("REQUEST_METHOD=") + request.getMethod();
	queryString = std::string("QUERY_STRING=") + response.getQueryString();
	uploadDir = std::string("UPLOAD_DIR=") + UPLOAD_DIR;

	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("REDIRECT_STATUS=200");
	env.push_back(scriptFilename.c_str());
	env.push_back(requestMethod.c_str());
	env.push_back(queryString.c_str());
	if (!conf.uploadDir.empty())
		uploadDir = std::string("UPLOAD_DIR=") + conf.uploadDir;
	env.push_back(uploadDir.c_str());
	if (request.getMethod() == "POST")
	{
		std::stringstream ss;
		ss << "CONTENT_LENGTH=" << request.getContentLength();
		contentLength = ss.str();
		if (ss.fail())
		{
			this->env = std::vector< const char * >();
			return(response.error(INTERNAL, "cannot convert body length meta var"));
		}
		env.push_back(contentLength.c_str());
		contentType = "CONTENT_TYPE=" + request.getContentType();
		env.push_back(contentType.c_str());
	}
	env.push_back(NULL);
}

void CGI::execCGI(Request & request, Response & response)
{
	//- + Redirige tmpFile en input pour que POST puisse prendre le body en input
	int fd_tmp_file_in = open(request.tmpFilename.c_str(), O_RDONLY);
	if (dup2(fd_tmp_file_in, STDIN_FILENO) == -1)
		return (response.error(INTERNAL, "dup2 1 syscall failed"));
	//- Redirige stdout pour recuperer la reponse du cgi
	int fd_tmp_file_out = open(response.tmpFilename.c_str(), O_WRONLY);
	if (dup2(fd_tmp_file_out, STDOUT_FILENO) == -1)
		return (response.error(INTERNAL, "dup2 syscall failed"));
	//- Se place dans le bon directory et exec php cgi
	if (chdir(response.getPath().substr(0, response.getPath().rfind("/") - 1).c_str()) == -1)
		return (response.error(NOT_FOUND, "chdir syscall failed"));
	if (execve(*args.begin(), (char* const*)&(*args.begin()), (char* const*)&(*env.begin())) == -1)
		return (response.error(INTERNAL, "execve syscall failed"));
}

void CGI::launchCGI(Request & request, Response & response, Conf & conf)
{
	initCGIArgs(response, conf);
	if (args.empty())
		return ;
	initCGIEnv(request, response, conf);
	if (env.empty())
		return ;
	//-- EXEC PHP-CGI : fork, redirige stdout pour recuperer la reponse cgi, execve cgi 
	pid_t pid = fork();
	if (pid == -1)
		return (response.error(INTERNAL, "fork syscall failed"));
	else if (pid == 0)
		execCGI(request, response);
	else
	{
		if (wait(NULL) == -1)
			return (response.error(INTERNAL, "wait syscall failed"));
	}
}
