#include "Conf.hpp"

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace Utils
{

/* SELECTIONNE DANS LE VECTOR DE CONF, EN FONCTION DU SERVERNAME LA CONF CORRESPONDANTE, PAR DEFAUT LA PREMIERE */
/*
Conf const& selectConf(std::vector<Conf> & confs, std::string const& server_name) {
	for (std::vector<Conf>::const_iterator it = confs.begin(); it != confs.end(); ++it)
	{
		std::vector<std::string> server_names = it->getServerName();
		for (std::vector<std::string>::const_iterator it2 = server_names.begin(); it2 != server_names.end(); ++it2)
			if (*it2 == server_name)
				return (*it);
	}
	return (confs[0]);
}
*/
Conf const& selectConf(std::vector<Conf> & confs, std::string const& server_name, std::string const& request_path) {
	for (std::vector<Conf>::iterator it = confs.begin(); it != confs.end(); ++it) // pour chaque conf dans confs
	{
		for (std::vector<std::string>::const_iterator it2 = it->serverName.begin(); it2 != it->serverName.end(); ++it2) // pour chaque server_name dans conf
		{
			if (*it2 == server_name) // la conf correspond au server_name
			{
				for (std::map<std::string, Conf>::const_iterator it3 = it->locations.begin(); it3 != it->locations.end(); ++it3) // pour chaque location dans conf
				{
					if (request_path.find(it3->first) == 0)	//if path parameter start with locationPath
						return (it3->second);
				}
				return (*it);
			}
		}
	}
	return (confs[0]);
}

std::size_t header_is_full(std::string & buf) {
	std::stringstream buffer(buf);
	std::string line;
	std::size_t header_size = 0;
	while (std::getline(buffer, line)){
		header_size += line.length() + 1;
		if (line == "\r" || line == "") // fin de header
			return header_size;
	}
	return 0;
}


} // namespace Utils
