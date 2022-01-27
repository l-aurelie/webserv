#include "Conf.hpp"
#include "Parser.hpp"
#include "Server.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdint.h>
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

	std::map< uint16_t, std::vector<Conf> > confs = Parser::parse_conf(path);

	std::vector<Server> servers;
	for (std::map<uint16_t, std::vector<Conf> >::iterator it = confs.begin(); it != confs.end(); it++){
		Server server(it->second);
		servers.push_back(server);
	}
	for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
		it->initServ(it->confs[0].getListen());

	while (true)
	{
		for(std::vector<Server>::iterator it = servers.begin(); it != servers.end(); it++)
			it->launch();
	}
	return (EXIT_SUCCESS);

	/*
	PRINT ALL MAP
	for (std::map< uint16_t, std::vector<Conf> >::iterator it = confs.begin(); it != confs.end(); ++it)
	{
		std::cout << "map[" << it->first << "] = ";
		for (std::vector<Conf>::iterator it2 = confs[it->first].begin(); it2 != confs[it->first].end(); ++it2)
			std::cout << *it2;
	}
	*/

	/* 
	AVEC PORT et SERVER_NAME *it2 retourne la confs correspondante
	for (std::vector<Conf>::iterator it2 = confs[6500].begin(); it2 != confs[6500].end(); ++it2)
	{
		std::vector<std::string> vec = it2->getServerName();
		if (std::find(vec.begin(), vec.end(), "antoine.localhost") != vec.end())
		{
			std::cout << *it2 << std::endl;
			break ;
		}
	}
	*/
}