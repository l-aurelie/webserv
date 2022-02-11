#pragma once

#include <ostream>
#include <string>
#include <vector>
#include <map>
#include <stdint.h>

class Conf
{
	public:
		Conf();
		Conf(int, std::vector<std::string>);
		Conf(Conf const& rhs);
		~Conf();
		Conf & operator=(Conf const &rhs);

		void setListen(std::vector<std::string> const& values);
		void setServerName(std::vector<std::string> const& values);
		void setAutoindex(std::vector<std::string> const& values);
		void setIndex(std::vector<std::string> const& values);
		void setRoot(std::vector<std::string> const& values);
		void setClientMaxBodySize(std::vector<std::string> const& values);
		void setReturn(std::vector<std::string> const& values);
		void setErrorPages(std::vector<std::string> const& values);

	//	uint16_t getListen() const;
	//	std::vector<std::string> getServerName() const;
	//	bool getAutoindex() const;
	//	std::vector<std::string> getIndex() const;
	//	std::string getRoot() const;
	//	std::size_t getClientMaxBodySize() const;

		uint16_t listen;
		std::vector<std::string> serverName;
		int autoindex;
		std::vector<std::string> index;
		std::string root;
		int clientMaxBodySize;
		std::map< std::string, Conf > locations;	//TODO: Private ?
		std::string locationPath;
		std::string redirectURL;
		int redirectCode;
		std::map<int, std::string> errorPages;
	private:
};

std::ostream & operator<<(std::ostream & os, Conf const& rhs);
