#pragma once

#include "Conf.hpp"
#include <vector>
#include <string>
#include <sys/stat.h>

namespace Utils
{

Conf & selectConf(std::vector<Conf> &confs, std::string const& server_name, std::string const& path);
std::string setTime(const time_t *timep);
std::string tolowerstr(std::string const& str);

}
