#include "Parser.hpp"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

namespace Parser
{

static void openConfigFile(std::string const& path, std::ifstream &readFile) {
	readFile.open(path.c_str());
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
	directives.insert(std::pair<std::string, void (Conf::*)(std::vector<std::string> const& )>("autoindex", &Conf::setAutoindex));
	directives.insert(std::pair<std::string, void (Conf::*)(std::vector<std::string> const& )>("index", &Conf::setIndex));
	directives.insert(std::pair<std::string, void (Conf::*)(std::vector<std::string> const& )>("root", &Conf::setRoot));
	directives.insert(std::pair<std::string, void (Conf::*)(std::vector<std::string> const& )>("client_max_body_size", &Conf::setClientMaxBodySize));

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
			if (ss.eof())
			{
				std::cerr << "error: config file: syntax error missing ';'" << std::endl;
				exit(EXIT_FAILURE);
			}
			vec.push_back(word);
			ss >> word;
		}
		(conf.*(directives[key]))(vec);
		ss >> word;
	}
	return (conf);
}

std::map< uint16_t, std::vector<Conf> > parse_conf(std::string const& path) {

	std::map< uint16_t, std::vector<Conf> > confs;
	Conf conf;
	std::ifstream readFile;
	std::stringstream ss;
	openConfigFile(path, readFile);
	ss << readFile.rdbuf();

	std::string buf;
	ss >> buf;
	while (!ss.eof())
	{
		std::stringstream blockStream;
		if (buf != "server"){ // maybe keywords before first "server {"
			std::cerr << "conf file erreur: no 'server' keyword \n";
			exit(EXIT_FAILURE);
		}
		ss >> buf;
		if (buf != "{"){
			std::cerr << "conf file erreur: no '{' after 'server' \n";
			exit(EXIT_FAILURE);
		}
		else if (buf == "{"){
			ss >> buf;
			while (!ss.eof() && buf != "}" && buf != "server" && buf != "{"){ 
				if (buf[buf.length() - 1] == ';'){
					buf = buf.substr(0, buf.length() - 1);
					blockStream << buf << " ; ";
				}
				else 
					blockStream << buf << " ";
				ss >> buf;
			}
			if (buf == "}"){
				conf = parse_directives(blockStream);
				confs[conf.getListen()].push_back(conf);
			}
			else{
				ss.str("");
				blockStream.str("");
				std::cerr << "conf file erreur, end wihtout '}' \n";
				exit(EXIT_FAILURE);
			}
		}
		ss >> buf;
	}
	readFile.close();
	return (confs);
}

} // namespace Parser
