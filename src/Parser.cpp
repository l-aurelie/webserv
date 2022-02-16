#include "Parser.hpp"
#include "Request.hpp"
#include "webserv.hpp"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>
#include <string> // std::getline() std::append()

namespace Parser
{

static void openConfigFile(std::string const& path, std::ifstream &readFile) {
	readFile.open(path.c_str());
	if (readFile.fail())
	{
		std::cerr << "error: cannot open config file '" << path << "'" << std::endl;
		exit(EXIT_FAILURE); // g_error
	}
}

//TODO: gestion error exit failure ? 
static void parseDirectives(std::stringstream & ss, Conf & conf) {
	std::string word;
	std::map<std::string, void (Conf::*)(std::vector<std::string> const&)> directives;

	directives["listen"] = &Conf::setListen;
	directives["server_name"] = &Conf::setServerName;
	directives["autoindex"] = &Conf::setAutoindex;
	directives["index"] = &Conf::setIndex;
	directives["root"] = &Conf::setRoot;
	directives["client_max_body_size"] = &Conf::setClientMaxBodySize;
	directives["return"] = &Conf::setReturn;
	directives["error_page"] = &Conf::setErrorPages;
	directives["allowed_methods"] = &Conf::setAllowedMethods;

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
}

void completeConfLocation(Conf & conf)
{
	//TODO: Rajouter les futures variable parsees
	/* Initialise les variables de la conf Server{} */
	if (conf.autoindex == -1)
		conf.autoindex = 0;
	if (conf.clientMaxBodySize == -1)
		conf.clientMaxBodySize = 0;
	if (conf.redirectCode == -1)
		conf.redirectCode = 0;
	if (conf.allowedMethods.empty())
	{
		conf.allowedMethods.push_back("GET");
		conf.allowedMethods.push_back("POST");
		conf.allowedMethods.push_back("DELETE");
	}

	/* Initialise les variables locations vides aux valeurs de la conf Server{} */
	for (std::map<std::string, Conf>::iterator it = conf.locations.begin(); it != conf.locations.end(); it++)
	{
		if(it->second.autoindex == -1)
			it->second.autoindex = conf.autoindex;
		if(it->second.index.empty())
			it->second.index = conf.index;
		if(it->second.root.empty())
			it->second.root = conf.root;
		if(it->second.clientMaxBodySize == -1)
			it->second.clientMaxBodySize = conf.clientMaxBodySize;
		if(it->second.redirectCode == -1)
			it->second.redirectCode = conf.redirectCode;
		if(it->second.redirectURL.empty())
			it->second.redirectURL = conf.redirectURL;
		if (it->second.allowedMethods.empty())
			it->second.allowedMethods = conf.allowedMethods;
		if (it->second.errorPages.empty())
			it->second.errorPages = conf.errorPages;
	}
}

//TODO: gestion erreurs exit failure
std::map< uint16_t, std::vector<Conf> > parseConf(std::string const& path)
{
	std::map< uint16_t, std::vector<Conf> > confs;
	std::ifstream readFile;
	std::stringstream ss;
	openConfigFile(path, readFile);
	ss << readFile.rdbuf();

	std::string buf;
	ss >> buf;
	//-- Lecture fichier conf
	while (!ss.eof())
	{
		Conf conf;
		std::stringstream blockServer;
		//-- Gestion erreur blockServ
		if (buf != "server") { // maybe keywords before first "server {"
			std::cerr << "conf file erreur: no 'server' keyword \n";
			exit(EXIT_FAILURE);
		}
		ss >> buf;
		if (buf != "{") {
			std::cerr << "conf file erreur: no '{' after 'server' \n";
			exit(EXIT_FAILURE);
		}
		//-- DECOUPE 1BLOCK-SERV
		else if (buf == "{") {	// '{'
			std::string record(buf);
			ss >> buf;
			while (!ss.eof() && buf != "}" && buf != "server" && buf != "{") {
				//-- DECOUPE BLOCK LOCATION
				if(buf == "location" && (record == "}" || record == "{" || record[record.length() - 1] == ';'))
				{
					std::stringstream blockLocation;
					//-- on recup le path de location
					ss >> buf;
					std::string location_path(buf); 
					ss >> buf;
					if (buf != "{") // tjrs '{' apres locationPath
					{
						std::cerr << "conf file erreur: no '{' after 'location' \n";
						exit(EXIT_FAILURE);
					}
					else if (buf == "{")
					{
						ss >> buf;
						//-- Concatene bloc location
						while (!ss.eof() && buf != "}" && buf != "{")
						{
							//-- Split " " et ";"
							if (buf[buf.length() - 1] == ';')
								blockLocation << buf.substr(0, buf.length() - 1) << " ; ";
							else
								blockLocation << buf << " ";
							ss >> buf;
						}
						//-- Decoupe location finie, parse location-conf, ajoute a la map locations presente dans la conf du blocserv
						if (buf == "}")
						{
							parseDirectives(blockLocation, conf.locations[location_path]);
							//conf.locations[location_path].locationPath = location_path;
						}
						else {
							std::cerr << "conf file erreur, location end wihtout '}' \n";
							exit(EXIT_FAILURE);
						}
					}
				}
				//-- Split " " et ";"
				else if (buf[buf.length() - 1] == ';')
				//if (buf[buf.length() - 1] == ';')
					blockServer << buf.substr(0, buf.length() - 1) << " ; ";
				else
					blockServer << buf << " ";
				record = buf;
				ss >> buf;
			}
			//-- Decoupe blocserv finie, parse bloc serv et ajoute a la map Confss
			if (!ss.eof() && buf == "}") {	// '}'
				parseDirectives(blockServer, conf);
				completeConfLocation(conf);
			//	std::cerr << "Location[\"" << locationPath << "\"] = " << conf.locations[locationPath] << std::endl;
				// avant de push_back, completer conf.locations
				confs[conf.listen].push_back(conf);
			}
			else {
				std::cerr << "conf file erreur, end wihtout '}' \n";
				exit(EXIT_FAILURE); 
			}
		}
		ss >> buf;
	}
	return (confs);
}

std::string tolowerstr(std::string str){
	std::string ans;
	for (size_t i = 0; i < str.length(); i++){
		ans.append(1, tolower(str[i]));
	}
	return ans;
}

//TODO: parse plusieurs line
static Request & parseFields(std::stringstream & header_buf, Request & request) {
	std::string word;
	std::string line;
	std::string key;
	std::map<std::string, void (Request::*)(std::vector<std::string> &)> fields;

	//std::cout << "param parseFileds: |" << header_buf.str() << "|" << std::endl;
	fields["host"] = &Request::setHost;
	fields["content-length"] = &Request::setContentLength;
	fields["content-type"] = &Request::setContentType;
	fields["user-agent"] = NULL;

	fields["origin"] = NULL;

	fields["connection"] = NULL;
	fields["pragma"] = NULL;
	fields["cache-control"] = NULL;

	fields["accept"] = NULL;
	fields["accept-language"] = NULL;
	fields["accept-encoding"] = NULL;

	fields["upgrade-insecure-requests"] = NULL;

	fields["sec-fetch-dest"] = NULL;
	fields["sec-fetch-mode"] = NULL;
	fields["sec-fetch-site"] = NULL;
	fields["sec-fetch-user"] = NULL;

	fields["referer"] = NULL;

	std::getline(header_buf, line);//pour clear la "premiere ligne"
	while (std::getline(header_buf, line) && line != "\r")//tant que pas fin header_buff
	{
		std::vector<std::string> values;
		//std::cout << "line = " << (int)line[0] << std::endl;
		if(line.find(":") == std::string::npos)// si un : else erreur
			return (request.errorMsg(BAD_REQUEST, "syntax error ':' not found"));
		std::stringstream ss;
		ss << line;
		ss >> key; // on met le premier mot dans key
		key = tolowerstr(key);//insensible a la casse
		if (key[key.length() - 1] != ':') // key doit toujours se terminer par ':'
			return request.errorMsg(BAD_REQUEST, "syntax error key must be followed by ':'");
		key = key.substr(0, key.length() - 1);
		if (fields.count(key) <= 0)//si la clef nextiste pas dans map
			return (request.errorMsg(BAD_REQUEST, std::string("unknow keyword '" + key + "'").c_str()));
		while(!ss.eof() && ss >> word)// tant que pas fin ligne on ajoute les mot dans la vector value
			values.push_back(word);
		if (fields[key])
			(request.*(fields[key]))(values);
	}
	return request;
}
			/*
			//key << word;
	//		while(word != ":")//tant que != ":" met dans key
	//		{
	//			key << word;
	//		}
			else
			{
				ss >> word; // key [vide]
				while(!ss.eof())// tant que pas fin ligne on ajoute les mot dans la vector value
				{
					values.push_back(word);
					ss >> word;	
				}
				while(getline(header_buf, line))
				{
					if(line.find(":") != std::string::npos)
						break;
					if(line[0] != '\t' && line[0] != ' ')
					{
						std::cerr << "Bad request : multiple lines must begin with tabulation or space" << std::endl;
						exit(EXIT_FAILURE);//TODO GESTION ERREUR REQUEST STATUS
					}
					ss << line;
					ss >> word;
					while(!ss.eof())// tant que pas fin ligne on ajoute les mot dans la vector value
					{
						values.push_back(word);
						ss >> word;	
					}				
				}
			}
			(request.*(fields[key.str()]))(values);
			std::cout << "request = |" << request << "|" << std::endl;	// debug curr request
		}
		else//la premiere ligne contient pas :
		{
			std::cerr << "Bad request : syntax in header request : usage [key] : [value] or [key] : [value1] [value2] [...] \n [value3]" << std::endl;
			exit(EXIT_FAILURE);//TODO GESTION ERREUR REQUEST STATUS
		}
	}
	return (request);
}
*/

Request & parseRequest(Request & request){
	std::stringstream ss(request.headerBuf);
	std::string buf;

	//std::cout << "param parseRequest: |" << ss.str() << "|" << std::endl;
	ss >> buf;
	if (buf != "POST" && buf != "GET" && buf != "DELETE")
		return (request.errorMsg(BAD_REQUEST, std::string("unknow method '" + buf + "'").c_str()));
	request.setMethod(buf);
	ss >> buf;
	// path: file to add with root => [root]/[index]
	if (buf[0] != '/') // path must start with /
		return (request.errorMsg(BAD_REQUEST, "path must start with '/'"));
	request.setPath(buf);
	ss >> buf;
	if (buf != "HTTP/1.1") // protocol version
		return (request.errorMsg(BAD_REQUEST, "protocol version not supported, only HTTP/1.1 works"));
	request.setProtocolVersion(buf);
	return parseFields(ss, request); // ss curseur sur la 2 eme ligne
}

} // namespace Parser
