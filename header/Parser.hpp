#pragma once

#include "Conf.hpp"
#include <vector>

namespace Parser {

std::vector<Conf>	parse(std::string const& path);

}