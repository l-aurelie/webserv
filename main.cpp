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

#define BUF_SIZE 1024

static int error(const char *str)
{
	std::cerr << str << std::endl;
	return 1;
}

int main(void)
{
	std::string path("/home/aurelie/Documents/webserv/site///phpinfo.php");

	std::stringstream ss;
	std::string budy;
	int fds_out[2];

	if (pipe(fds_out) == -1) // OUVERTURE PIPE_OUT ====
		exit(error("error: pipe syscall failed"));

/* Rempli les arguments passes a exec */
	std::vector<const char *>args;
	std::cerr << "At begining of launchCGI, path is: " << path << std::endl;
	std::string tmp = path.substr(0, path.rfind("/")) + "../cgi/php-cgi";
	//std::cerr << "exec is " << (path.substr(0, path.rfind("/")) + "../cgi/php-cgi") << std::endl;
	//args.push_back(tmp.c_str());	// TODO:
	//args.push_back(path.c_str());
	args.push_back("../cgi/php-cgi");
	args.push_back("./phpinfo.php");
	args.push_back(NULL);
	
/* Rempli les meta var */
	std::vector<const char *>env;
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("REDIRECT_STATUS=true");
	env.push_back("SCRIPT_FILENAME=./phpinfo.php");

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

	pid_t pid = fork();
	if (pid == -1)
		exit(error("error: fork syscall failed"));
	else if (pid == 0)
	{
	
		if (close(fds_out[0]) == -1)
			exit(error("error: close syscall failed"));
		if (dup2(fds_out[1], STDOUT_FILENO) == -1)
			exit(error("error: dup2 syscall failed"));
		if (chdir("./site/") == -1) // TODO: path ? //chdir(root/path - phpinfo.php);
			exit(error("error: chdir syscall failed"));
		// root/dir/file

		std::cerr << "==============" << std::endl;
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
		std::cerr << "==============" << std::endl;

/* Exec php-cgi */ 
		//std::cerr << "execve(" << *args.begin() << ", " << (*(args.begin() + 1)) << ", " << (*env.begin()) << ")" << std::endl;

		const char * argt[3] = {
			"/home/aurelie/Documents/webserv/cgi/php-cgi",
			"/home/aurelie/Documents/webserv/site/phpinfo.php",
			NULL
		};
		/*
		const char * envt[6] = {
			"GATEWAY_INTERFACE=CGI/1.1",
			"REDIRECT_STATUS=true",
			"SCRIPT_FILENAME=/home/aurelie/Documents/webserv/site///phpinfo.php",
			"REQUEST_METHOD=GET",
			"QUERY_STRING=toto=6&send=send",
			NULL,
		};
		*/
		const char * envt[6] = {
			"REDIRECT_STATUS=true",
			"REQUEST_METHOD=GET",
			"QUERY_STRING=prenom=antoine&nom=gautier",
			"SCRIPT_FILENAME=/home/aurelie/Documents/webserv/site/phpinfo.php",
			"GATEWAY_INTERFACE=CGI/1.1",
			NULL
		};

		std::cerr << "execve(" << argt[0] << ", " << argt[0] << ", " << argt[1] << ", " << (*env.begin()) << ")" << std::endl;
		(void)envt;
        //if (execve(argt[0], (char* const*)argt, (char* const*)envt) == -1)
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
	}
	std::cerr << "================" << std::endl;
	std::cerr << "budy is |\n" << budy << "|\n";
	std::cerr << "================" << std::endl;

	std::string contentType = budy.substr(budy.find(": ") + 2, std::string::npos);
	contentType = contentType.substr(0, contentType.find("\n"));
	budy = budy.substr(budy.find("\n") + 1);	// enlever la ligne content-type
	budy = budy.substr(budy.find("\n") + 1);	// enlever la ligne vide

	std::cout << "--------\n";
	std::cout << "contenttype in CGI: " << contentType << '\n';
	std::cout << "body in CGI: " << budy << '\n';
	std::cout << "--------\n";



	std::cout << "RETURN IS " << budy << std::endl;
	return (EXIT_SUCCESS);
}