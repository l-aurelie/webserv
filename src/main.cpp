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
#include <unistd.h>

int	main(int argc, char **argv) {
	/* GESTION ARGS (file conf) */
	if (argc > 2)
	{
		std::cerr << "Usage ./webserv [path_to_config_file]" << std::endl;
		return (EXIT_FAILURE);// g_error: exit
	}

	std::string path = "./conf/default.conf";
	if(argc == 2)
		path = argv[1];

	/* PARSE CONFIGURATION dans map confs 1port =  1vecteur de Conf */
	std::map< uint16_t, std::vector<Conf> > confs = Parser::parseConf(path);

	std::vector<Server> servers;
	for (std::map<uint16_t, std::vector<Conf> >::iterator it = confs.begin(); it != confs.end(); it++){
		Server server(it->second);
		servers.push_back(server);
	}

	/* LANCE UN SERVER POUR CHAQUE PORT */
	for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
	{
		if (!it->initServ(it->getConfs()[0].getListen()))
		{
			std::cerr << "initServ failed" << std::endl;
			return (EXIT_FAILURE);
		}
	}

	/* Chacun leur tour les server ecoutent les connections et les requetes */
	while (true)
	{
		for(std::vector<Server>::iterator it = servers.begin(); it != servers.end(); it++){
			it->launch();
		}
		usleep(500);
	}
	std::cerr << "main ended" << std::endl;
	return (EXIT_SUCCESS); // g_error

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
