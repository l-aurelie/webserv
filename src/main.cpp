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
	std::map< uint16_t, std::vector<Conf> > confss = Parser::parseConf(path);
	// debug print all conf
	/*
	for (std::map< uint16_t, std::vector<Conf> >::const_iterator it_ = confss.begin(); it_ != confss.end(); ++it_)
	{
		std::cout << "confss[" << it_->first << "] = ";
		std::vector<Conf> vec = it_->second;
		for (std::vector<Conf>::const_iterator itii2 = vec.begin(); itii2 != vec.end(); ++itii2)
			std::cout << "aaa" << std::endl;
	}
	*/
	/*
	std::map< uint16_t, std::vector< Conf > >::const_iterator confssit;
	for (confssit = confss.begin(); confssit != confss.end(); ++confssit)
	{
		std::cout << "confS found : " << confssit->first << std::endl;	// TODO: without printing confssit->first, it works
		std::vector< Conf > confs = confssit->second;
		std::vector< Conf >::const_iterator confsit;
		for (confsit = confs.begin(); confsit != confs.end(); ++confsit)
		{
			std::cout << "conf found" << std::endl;
		}
	}
	*/

//	exit(1);
	//	confss = map < PORT, confs>	=> MAIN => MAP< PORT, confs >
	//	confs = vector< Conf > => Toutes les configurations d'un seul PORT
	//	conf = block Server

	std::vector<Server> servers;
	for (std::map<uint16_t, std::vector<Conf> >::iterator it = confss.begin(); it != confss.end(); it++){
		Server server(it->second);
		servers.push_back(server);
	}

	/* LANCE UN SERVER POUR CHAQUE PORT */
	for (std::vector<Server>::iterator it = servers.begin(); it != servers.end(); ++it)
	{
		if (!it->initServ(it->getConfs()[0].listen))
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
	//	usleep(500);
	}
	std::cerr << "main ended" << std::endl;
	return (EXIT_SUCCESS); // g_error

	/*
	PRINT ALL CONF MAP 
	for (std::map< uint16_t, std::vector<Conf> >::iterator it = confss.begin(); it != confss.end(); ++it)
	{
		std::cout << "confss[" << it->first << "] = ";
		for (std::vector<Conf>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
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
