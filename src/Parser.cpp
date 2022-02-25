#include "Parser.hpp"
#include "Request.hpp"
#include "Utils.hpp"
#include "webserv.hpp"

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

static void error(std::string const& msg)
{
	std::cerr << "error (config file): " << msg << std::endl;
	exit(EXIT_FAILURE);
}

/* PARSE UN BLOC DE CONFIGURATION DANS UN OBJET CONF */
static void parseDirectives(std::stringstream & ss, Conf & conf)
{
	std::string word;
	std::map<std::string, void (Conf::*)(std::vector<std::string> const&)> directives;

	directives["allowed_methods"] = &Conf::setAllowedMethods;
	directives["autoindex"] = &Conf::setAutoindex;
	directives["client_max_body_size"] = &Conf::setClientMaxBodySize;
	directives["error_page"] = &Conf::setErrorPages;
	directives["index"] = &Conf::setIndex;
	directives["listen"] = &Conf::setListen;
	directives["return"] = &Conf::setReturn;
	directives["root"] = &Conf::setRoot;
	directives["server_name"] = &Conf::setServerName;
	directives["upload_dir"] = &Conf::setUploadDir;
	directives["cgi"] = &Conf::setCGI;

	ss >> word;
	while (!ss.eof())
	{
		std::string key;
		std::vector<std::string> vec;

		key = word;
		if (directives.count(key) <= 0)
			error(std::string("unknow config file keyword '") + word  + "'");

		ss >> word;
		while (word != ";")
		{
			if (ss.eof())
				error("syntax error missing ';'");
			vec.push_back(word);
			ss >> word;
		}
		(conf.*(directives[key]))(vec);
		ss >> word;
	}
}

/* LA LOCATION COPIE LES VARIABLES NON INITIALISEE A PARTIR DE SA CONF PARENTE */
void completeConfLocation(Conf & conf)
{
	//-- Initialise les variables de la conf Server{}
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

	//-- Initialise les variables locations vides aux valeurs de la conf Server{}
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

/* PARSE LES CONFS ET LEURS LOCATIONS, RENVOIE UNE MAP CONFSS, CONTENANT UN PORT ET TOUTES SES CONFS CORRESPONDANTES */
std::map< uint16_t, std::vector<Conf> > parseConf(std::string const& path)
{
	std::map< uint16_t, std::vector<Conf> > confs;
	std::ifstream readFile;
	std::stringstream ss;
	readFile.open(path.c_str());
	if (readFile.fail())
		error(std::string("cannot open config file '") + path + "'");
	ss << readFile.rdbuf();

	std::string buf;
	ss >> buf;
	//-- Lecture fichier conf
	while (!ss.eof())
	{
		Conf conf;
		std::stringstream blockServer;
		//-- Gestion erreur blockServ
		if (buf != "server")
			error("no 'server' keyword");
		ss >> buf;
		if (buf != "{")
			error("no '{' after 'server' keyword");
		//-- DECOUPE 1 BLOCK-SERV
		else if (buf == "{")
		{
			std::string record(buf);
			ss >> buf;
			while (!ss.eof() && buf != "}" && buf != "server" && buf != "{")
			{
				//-- DECOUPE 1 BLOCK LOCATION
				if(buf == "location" && (record == "}" || record == "{" || record[record.length() - 1] == ';'))
				{
					std::stringstream blockLocation;
					//-- On recupere le path de location
					ss >> buf;
					std::string location_path(buf);
					ss >> buf;
					if (buf != "{") // tjrs '{' apres locationPath
						error("no '{' keyword");
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
						//-- Decoupe de location finie, parse location-conf, ajoute a la map locations presente dans la conf du blocserv
						if (buf == "}")
							parseDirectives(blockLocation, conf.locations[location_path]);
						else
							error("location end wihtout '}'");
					}
				}
				//-- Split " " et ";"
				else if (buf[buf.length() - 1] == ';')
					blockServer << buf.substr(0, buf.length() - 1) << " ; ";
				else
					blockServer << buf << " ";
				record = buf;
				ss >> buf;
			}
			//-- Decoupe blocserv finie, parse bloc serv et ajoute a la map Confss
			if (!ss.eof() && buf == "}")
			{
				parseDirectives(blockServer, conf);
				completeConfLocation(conf);
				// avant de push_back, completer conf.locations
				confs[conf.listen].push_back(conf);
			}
			else
				error("end wihtout '}'");
		}
		ss >> buf;
	}
	return (confs);
}

/* PARSE LE HEADER DE LA REQUETE DANS UN OBJET REQUEST */
static Request & parseFields(std::stringstream & header_buf, Request & request)
{
	std::string word;
	std::string line;
	std::string key;
	std::map<std::string, void (Request::*)(std::vector<std::string> &)> fields;

	fields["host"] = &Request::setHost;
	fields["content-length"] = &Request::setContentLength;
	fields["content-type"] = &Request::setContentType;
	fields["transfer-encoding"] = &Request::setTransferEncoding;

	fields["accept"] = NULL;
	fields["accept-encoding"] = NULL;
	fields["accept-language"] = NULL;
	fields["cache-control"] = NULL;
	fields["connection"] = NULL;
	fields["expect"] = NULL;
	fields["if-modified-since"] = NULL;
	fields["origin"] = NULL;
	fields["postman-token"] = NULL;
	fields["pragma"] = NULL;
	fields["referer"] = NULL;
	fields["sec-fetch-dest"] = NULL;
	fields["sec-fetch-mode"] = NULL;
	fields["sec-fetch-site"] = NULL;
	fields["sec-fetch-user"] = NULL;
	fields["upgrade-insecure-requests"] = NULL;
	fields["user-agent"] = NULL;

	std::getline(header_buf, line);	//pour clear la "premiere ligne"
	while (std::getline(header_buf, line) && line != "\r")
	{
		std::vector<std::string> values;
		if(line.find(":") == std::string::npos)
			return (request.errorMsg(BAD_REQUEST, (std::string("syntax error ':' not found in line '") + line + "'").c_str()));//TODO:
		std::stringstream ss(line);
		ss >> key;
		key = Utils::tolowerstr(key);
		if (key[key.length() - 1] != ':')
			return request.errorMsg(BAD_REQUEST, "syntax error key must be followed by ':'");
		key = key.substr(0, key.length() - 1);
		if (fields.count(key) <= 0)
			return (request.errorMsg(BAD_REQUEST, std::string("unknow keyword '" + key + "'").c_str()));
		while(!ss.eof() && ss >> word)
			values.push_back(word);
		if (fields[key])
			(request.*(fields[key]))(values);
	}
	return request;
}

Request & parseRequest(Request & request)
{
	std::stringstream ss(request.headerBuf);
	std::string buf;

	ss >> buf;
	if (buf != "POST" && buf != "GET" && buf != "DELETE")
		return (request.errorMsg(BAD_REQUEST, std::string("unknow method '" + buf + "'").c_str()));
	request.setMethod(buf);
	ss >> buf;
	if (buf[0] != '/') // path must start with /
		return (request.errorMsg(BAD_REQUEST, "path must start with '/'"));
	request.setPath(buf);
	ss >> buf;
	if (buf != "HTTP/1.1")
		return (request.errorMsg(BAD_REQUEST, "protocol version not supported, only HTTP/1.1 works"));
	request.setProtocolVersion(buf);
	return parseFields(ss, request); // ss curseur sur la 2 eme ligne
}

} // namespace Parser
