#pragma once

#include "Conf.hpp"
#include <map>
#include <stdint.h>
#include <vector>

namespace Parser {

std::map< uint16_t, std::vector<Conf> > parse_conf(std::string const& path);

}