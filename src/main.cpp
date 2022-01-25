#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include "Server.hpp"
#include "webserv.hpp"

int main(void) {
	Server webserv;

	webserv.initServ();
	webserv.launch();
	return (EXIT_SUCCESS);
}