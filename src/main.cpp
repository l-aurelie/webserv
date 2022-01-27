#include "Conf.hpp"
#include "Parser.hpp"
#include "Server.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int	main(int argc, char **argv) {
	if (argc > 2)
	{
		std::cerr << "Usage ./webserv [path_to_config_file]" << std::endl;
		return (EXIT_FAILURE);
	}

	std::string path = "./conf/default.conf";
	if(argc == 2)
		path = argv[1];

	std::vector<Conf> confs = Parser::parse_conf(path);
//	for (std::vector<Conf>::iterator it = confs.begin(); it != confs.end(); ++it)
//		std::cout << *it << std::endl;
	
	Server webserv;
	webserv.initServ(20000);

	Server webserv2;
	webserv2.initServ(20001);

	while (true)
	{
		webserv.launch();
		webserv2.launch();
	}
	return (EXIT_SUCCESS);
}