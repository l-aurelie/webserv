#pragma once

#include <ostream>
#include <string>
#include <vector>

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

		int getListen() const;
		std::vector<std::string> getServerName() const;

	private:
		int listen;
		std::vector<std::string> serverName;
};

std::ostream & operator<<(std::ostream & os, Conf const& rhs);
