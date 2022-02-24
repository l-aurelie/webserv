#include "Conf.hpp"

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace Utils
{

/* SELECTIONNE DANS LE VECTOR DE CONF, EN FONCTION DU SERVERNAME LA CONF CORRESPONDANTE, PAR DEFAUT LA PREMIERE */
Conf & selectConf(std::vector<Conf> & confs, std::string const& server_name, std::string const& request_path)
{
	for (std::vector<Conf>::iterator it = confs.begin(); it != confs.end(); ++it) // pour chaque conf dans confs
	{
		for (std::vector<std::string>::const_iterator it2 = it->serverName.begin(); it2 != it->serverName.end(); ++it2) // pour chaque server_name dans conf
		{
			if (*it2 == server_name) // la conf correspond au server_name
			{
				for (std::map<std::string, Conf>::iterator it3 = it->locations.begin(); it3 != it->locations.end(); ++it3) // pour chaque location dans conf
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

std::string setTime(const time_t *timep)
{
	struct tm* timeinfo = localtime(timep);
	char buf[30];
	strftime(buf, 30, "%a, %d %h %Y %T %Z", timeinfo);
	return buf;
}

std::string tolowerstr(std::string const& str)
{
	std::string ans;
	for (size_t i = 0; i < str.length(); i++)
		ans.append(1, tolower(str[i]));
	return ans;
}

} // namespace Utils
