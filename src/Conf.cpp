#include "Conf.hpp"
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <stdint.h>
#include <vector>

Conf::Conf(void) : listen(0), autoindex(-1), clientMaxBodySize(-1) { }

Conf::Conf(Conf const& rhs) { *this = rhs; }

Conf::~Conf(void) { }

Conf& Conf::operator=(Conf const& rhs) {
	if (this == &rhs)
		return (*this);
	this->listen = rhs.listen;
	this->serverName = rhs.serverName;
	this->autoindex = rhs.autoindex;
	this->index = rhs.index;
	this->root = rhs.root;
	this->clientMaxBodySize = rhs.clientMaxBodySize;
	this->locations = rhs.locations;
	this->locationPath = rhs.locationPath;
	return (*this);
}

std::ostream & operator<<(std::ostream & os, Conf const& rhs) {
	os << "Server {" << std::endl;

	if (rhs.listen)
		os << "\tlisten: " << rhs.listen << ";" << std::endl;

	if (rhs.serverName.size())
	{
		os << "\tserver_name:";
		for (std::vector<std::string>::const_iterator it = rhs.serverName.begin(); it != rhs.serverName.end(); ++it)
			os << " " << *it;
		os << ";" << std::endl;
	}
	
	if (!rhs.index.empty())
	{
		os << "\tindex:";
		for (std::vector<std::string>::const_iterator it = rhs.index.begin(); it != rhs.index.end(); ++it)
			os << " " << *it;
		os << ";" << std::endl;
	}

	if (!rhs.root.empty())
		os << "\troot: " << rhs.root << ";\n";

	if (rhs.autoindex)
		os << "\tautoindex: on;" << std::endl;

	if (rhs.clientMaxBodySize)
		os << "\tclientMaxBodySize: " << rhs.clientMaxBodySize << ";\n";
	
	for(std::map<std::string, Conf>::const_iterator it = rhs.locations.begin(); it != rhs.locations.end(); it++)
		std::cout << "location " << it->first << " " << it->second;
	// boucle pour chaque element de la map
		// affiche la key et la value

	os << "}" << std::endl;
	return (os);
}

void Conf::setListen(std::vector<std::string> const& values) {
	if (values.size() != 1)
	{
		std::cerr << "error: config file : 'listen' accepts only one port" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (!std::isdigit(values[0][0]))
	{
		std::cerr << "error: config file : 'listen' unsigned numeric value required" << std::endl;
		exit(EXIT_FAILURE);
	}
	std::stringstream ss;
	ss << values[0];
	ss >> this->listen;
	if (ss.fail())
	{
		std::cerr << "error: config file : 'listen' unsigned numeric value required" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void Conf::setServerName(std::vector<std::string> const& values) { this->serverName = values; }
void Conf::setIndex(std::vector<std::string> const& values) { this->index = values; }

void Conf::setAutoindex(std::vector<std::string> const& values) {
	if (values.size() != 1)
	{
		std::cerr << "error: config file : 'autoindex' accepts only one value" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (values[0] == "on")
		this->autoindex = true;
	else if (values[0] == "off")
		this->autoindex = false;
	else
	{
		std::cerr << "error: config file : 'autoindex' should be only 'on' or 'off'" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void Conf::setRoot(std::vector<std::string> const& values) {
	if (values.size() != 1)
	{
		std::cerr << "error: config file : 'root' accepts only one path" << std::endl;
		exit(EXIT_FAILURE);
	}
	this->root = values[0];
}

void Conf::setClientMaxBodySize(std::vector<std::string> const& values) {	// TODO: gerer les unites
	if (values.size() != 1)
	{
		std::cerr << "error: config file : 'max_client_body_size' accepts only one value" << std::endl;
		exit(EXIT_FAILURE);
	}
	if (!std::isdigit(values[0][0]))
	{
		std::cerr << "error: config file : 'max_client_body_size' unsigned numeric value required" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::stringstream ss;
	ss << values[0];
	ss >> this->clientMaxBodySize;
	if (ss.fail())
	{
		std::cerr << "error: config file : 'max_client_body_size' unsigned numeric value required" << std::endl;
		exit(EXIT_FAILURE);
	}
}

//uint16_t Conf::getListen() const { return (this->listen); }
//std::vector<std::string> Conf::getServerName() const { return (this->serverName); }
//bool Conf::getAutoindex() const { return (this->autoindex); }
//std::vector<std::string> Conf::getIndex() const {return (this->index); }
//std::string Conf::getRoot() const { return (this->root); }
//std::size_t Conf::getClientMaxBodySize() const { return (this->clientMaxBodySize); }
