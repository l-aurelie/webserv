#include "Conf.hpp"
#include "Parser.hpp"
#include "Server.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <map>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <string>
#include <unistd.h>
#include <vector>

static std::string get_conf_path(int argc, char **argv)
{
	if (argc > 2)
	{
		std::cerr << "Usage ./webserv [path_to_config_file]" << std::endl;
		return ("");
	}

	std::string path = "./conf/default.conf";
	if(argc == 2)
		path = argv[1];
	return (path);
}

int	main(int argc, char **argv)
{
	std::string path = get_conf_path(argc, argv);
	if (path.empty())
		return (EXIT_FAILURE);

	//-- PARSE CONFIGURATION dans map confss 1port = 1vecteur de Conf
	std::map< uint16_t, std::vector<Conf> > confss = Parser::parseConf(path);

	std::vector<Server> servers;
	for (std::map<uint16_t, std::vector<Conf> >::iterator it = confss.begin(); it != confss.end(); it++)
	{
		Server server(it->second);
		servers.push_back(server);
	}
	//-- Lance un server pour chaque port
	for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
	{
		if (!it->initServ(it->getConfs()[0].listen))
		{
			std::cerr << "initServ failed" << std::endl;
			return (EXIT_FAILURE);
		}
	}

	//-- Chacun leur tour les server ecoutent les connections et les requetes
	while (true)
	{
		for(std::vector<Server>::iterator it = servers.begin(); it != servers.end(); it++)
			it->launch();
		usleep(500);
	}
	return (EXIT_SUCCESS);
}
