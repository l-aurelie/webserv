#pragma once

#include "Conf.hpp"
#include <vector>

namespace Parser {

std::vector<Conf>	parse_conf(std::string const& path);

}