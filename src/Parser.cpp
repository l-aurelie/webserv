#include "Parser.hpp"
#include "Request.hpp"
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
		exit(EXIT_FAILURE);
	}
}

static Conf parseDirectives(std::stringstream & ss) {
	Conf conf;
	std::string word;
	std::map<std::string, void (Conf::*)(std::vector<std::string> const&)> directives;

	directives["listen"] = &Conf::setListen;
	directives["server_name"] = &Conf::setServerName;
	directives["autoindex"] = &Conf::setAutoindex;
	directives["index"] = &Conf::setIndex;
	directives["root"] = &Conf::setRoot;
	directives["client_max_body_size"] = &Conf::setClientMaxBodySize;

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

std::map< uint16_t, std::vector<Conf> > parseConf(std::string const& path) {

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
				conf = parseDirectives(blockStream);
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

static std::string tolowerstr(std::string str){
	std::string ans;
	for (size_t i = 0; i < str.length(); i++){
		ans.append(1, tolower(str[i]));
	}
	return ans;
}

//TODO : parse plusieurs line
static Request parseFields(std::stringstream & header_buf, Request & request) {
	std::string word;
	std::string line;
	std::string key;
	std::vector<std::string> values;
	std::map<std::string, void (Request::*)(std::vector<std::string> &)> fields;

	std::cout << "param parseFileds: |" << header_buf.str() << "|" << std::endl;
	fields["host"] = &Request::setHost;
	fields["user-agent"] = NULL;

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
//	std::cout << "1st line = " << line << std::endl;
	while (std::getline(header_buf, line) && line != "\r")//tant que pas fin header_buff
	{
//		std::cout << "line = " << line << std::endl;
		//std::cout << "line size = " << line.length() << ": " << (int)line[0] << std::endl;
		if(line.find(":") == std::string::npos)// si un : else erreur
		{
			std::cerr << "Bad request: syntax error ':' not found" << std::endl;
			exit(EXIT_FAILURE);//TODO gestion erreur BAD REQUEST
		}
		std::stringstream ss;
		ss << line;
		ss >> key; // on met le premier mot dans key
		key = tolowerstr(key);//insensible a la casse
		if (key[key.length() - 1] != ':') // key doit toujours se terminer par ':'
		{
			std::cerr << "Bad request: syntax error key must be followed by ':'" << std::endl;
			exit(EXIT_FAILURE);
		}
		key = key.substr(0, key.length() - 1);
		if (fields.count(key) <= 0)//si la clef nextiste pas dans map
		{
			std::cerr << "error: request: unknow keyword '" << key << "'" << std::endl;
			exit(EXIT_FAILURE);//TODO gestion erreur BAD REQUEST
		}
		while(!ss.eof() && ss >> word)// tant que pas fin ligne on ajoute les mot dans la vector value
			values.push_back(word);

		if (fields[key])
			(request.*(fields[key]))(values);
	}
	std::cout << request << std::endl;	// debug curr request
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

Request parseRequest(char *requestMsg){
	std::stringstream ss;
	std::string buf;
	Request request;

	ss << requestMsg;
	std::cout << "param parseRequest: |" << ss.str() << "|" << std::endl;
	ss >> buf;
	if (buf != "POST" && buf != "GET" && buf != "DELETE"){
		std::cerr << "unknown method\n";
		exit(EXIT_FAILURE); // return msg to client
	}
	else
		request.setMethod(buf);
	ss >> buf;
	// path: file to add with root => [root]/[index]
	if (buf[0] != '/') // path must start with /
		exit(EXIT_FAILURE); // return msg to client
	else
		request.setPath(buf);
	ss >> buf;
	if (buf != "HTTP/1.1") // protocol version
		exit(EXIT_FAILURE); // return msg to client
 	else
		request.setProtocolVersion(buf);
	parseFields(ss, request); // ss curseur sur la 2 eme ligne
	return request;
}

} // namespace Parser
