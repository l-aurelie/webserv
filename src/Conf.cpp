#include "Conf.hpp"
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>

Conf::Conf(void) { }

Conf::Conf(Conf const& rhs) { *this = rhs; }

Conf::~Conf(void) { }

Conf& Conf::operator=(Conf const& rhs) {
	if (this == &rhs)
		return (*this);
	this->listen = rhs.listen;
	this->serverName = rhs.serverName;
	return (*this);
}

std::ostream & operator<<(std::ostream & os, Conf const& rhs) {
	std::vector<std::string> serverName = rhs.getServerName();

	os << "Server {" << std::endl;

	os << "\tlisten: " << rhs.getListen() << ";" << std::endl;

	if (serverName.size())
		os << "\tserver_name:";
	for (std::vector<std::string>::iterator it = serverName.begin(); it != serverName.end(); ++it)
		os << " " << *it;
	if (serverName.size())
		os << ";" << std::endl;

	os << "}" << std::endl;

	return (os);
}

void Conf::setListen(std::vector<std::string> const& values) {
	if (values.size() != 1)
	{
		std::cerr << "error: config file need only one port in listen directive" << std::endl;
		exit(EXIT_FAILURE);
	}

	std::stringstream ss;
	ss << values[0];
	ss >> this->listen;
	if (ss.bad() || ss.fail())
	{
		std::cerr << "error: config file need numeric value in listen directive" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void Conf::setServerName(std::vector<std::string> const& values) { this->serverName = values; }

inline int Conf::getListen() const { return (this->listen); }
inline std::vector<std::string> Conf::getServerName() const { return (this->serverName); }