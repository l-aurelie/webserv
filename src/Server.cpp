#include "Server.hpp"

Server::Server(void) {}
Server::Server(Server const& rhs) { *this = rhs; }
Server::~Server(void) {}

Server&	Server::operator=(Server const& rhs) {
	if (this == &rhs)
		return (*this);
	return (*this);
}
