#pragma once

#include "Conf.hpp"
#include "Request.hpp"
#include <map>
#include <stdint.h>
#include <vector>

namespace Parser
{

std::map< uint16_t, std::vector<Conf> > parseConf(std::string const& path);
Request & parseRequest(Request & request);
std::string tolowerstr(std::string & str);

}
