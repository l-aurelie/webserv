#pragma once

#include "Request.hpp"

namespace ServerUtils
{

void parseRecv(int bytes_read, char * buf, Request & req, std::vector< Conf > & confs);

}
