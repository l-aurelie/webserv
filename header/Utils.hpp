#pragma once

#include "Conf.hpp"
#include <vector>
#include <string>

namespace Utils
{

Conf const& selectConf(std::vector<Conf> &confs, std::string const& server_name);
std::size_t header_is_full(std::string & buf);

}
