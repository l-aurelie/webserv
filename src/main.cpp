#include <cstdlib>
#include <string>
#include "Server.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

int parse_conf(std::string path) {
	int port = -1;
	std::ifstream readFile(path.c_str());
	if (readFile.fail()){
		std::cerr << "open file read error \n";
		exit(EXIT_FAILURE);
	}
	std::string buf;
	std::stringstream ss;
	std::string garbage;
	int i = -1;
	while (i == -1 && std::getline(readFile, buf)){
		i = buf.find("listen");
	}
	buf.erase(0, i);
	ss << buf;
	ss >> garbage >> port;
	readFile.close();
	return (port);
}

int	main(int argc, char **argv) {
	if (argc > 2)
	{
		std::cerr << "Usage ./webserv [path_to_config_file]" << std::endl;
		return (EXIT_FAILURE);
	}

	std::string path = "./conf/default.conf";
	if(argc == 2)
		path = argv[1];

	int port = parse_conf(path);
	std::cout << "port = " << port << std::endl;
//	Server webserv;
//	webserv.initServ(port);
//	webserv.launch();
	return (EXIT_SUCCESS);
}