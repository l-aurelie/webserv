#include "Parser.hpp"
#include <cstdlib>
#include <string>
#include <fstream>
#include <map>
#include <sstream>
#include <iostream>
#include <vector>

namespace Parser
{

static void openConfigFile(std::string const& path, std::ifstream &readFile) {
	readFile.open(path.c_str());
	//readFile(path);
	if (readFile.fail())
	{
		std::cerr << "error: cannot open config file '" << path << "'" << std::endl;
		exit(EXIT_FAILURE);
	}
}

static Conf parse_directives(std::stringstream & ss) {
	Conf conf;
	std::string word;
	std::map<std::string, void (Conf::*)(std::vector<std::string> const&)> directives;

	directives.insert(std::pair<std::string, void (Conf::*)(std::vector<std::string> const& )>("listen", &Conf::setListen));
	directives.insert(std::pair<std::string, void (Conf::*)(std::vector<std::string> const& )>("server_name", &Conf::setServerName));

	//std::cout << "ss = |" << ss.str() << "|" << std::endl;
	ss >> word;
	while (!ss.eof())
	{
		std::string key;
		std::vector<std::string> vec;

		key = word;
		if (directives.count(key) <= 0)
		{
			std::cerr << "error: unknow config file keyword '" << word  << "'" << std::endl;
			exit(EXIT_FAILURE);
		}

		ss >> word;
		while (word != ";")
		{
			vec.push_back(word);
			ss >> word;
		}
		(conf.*(directives[key]))(vec);
		ss >> word;
	}
	return (conf);
}

std::vector<Conf> parse(std::string const& path) {

	std::vector<Conf> confs;
	std::ifstream readFile;
	std::stringstream ss;
	openConfigFile(path, readFile);
	ss << readFile.rdbuf();

	/*
		while(!ss.eof())
		{
			new_conf = parse_conf_bloc(return_conf_bloc(ss));
			confs.push_back(new_conf);
		}	
	*/
	std::string buf;
	ss >> buf;
	while (!ss.eof())
	{
		std::stringstream blockStream;
		if (buf != "server"){ // maybe keywords before first "server {"
			std::cerr << "conf file erreur: no 'server' keyword \n";
			break ;
		}
		ss >> buf;
		if (buf != "{"){
			std::cerr << "conf file erreur: no '{' after 'server' \n";
			break ;
		}
		else if (buf == "{"){
			ss >> buf;
			while (!ss.eof() && buf != "}" && buf != "server" && buf != "{"){ // inside a server block
				if (buf[buf.length() - 1] == ';'){ // word ending with ;, e.g. hello;
					buf = buf.substr(0, buf.length() - 1);
					blockStream << buf << " ; ";
				}
				else 
					blockStream << buf << " ";
				ss >> buf;
			}
			// stringstream blockStream either valid either == "";
			if (buf == "}"){
			//	std::cout << "blockstream: " << blockStream.str() << '\n';
				confs.push_back(parse_directives(blockStream));
			}
			else{
				ss.str("");
				blockStream.str("");
				std::cerr << "conf file erreur, end wihtout '}' \n";
				break ;
			}
		}
		ss >> buf;
	}

	readFile.close();
	return (confs);
}

} // namespace Parser
