#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <string>
#include <unistd.h>
#include "Server.hpp"
#include <poll.h>
#include <vector>


//struct pollfd *enlarge(struct pollfds *fds)
//{
//	struct pollfd *fds_new;
		

//}

int main(void) {
	Server server;

	int socketServer = socket(AF_INET, SOCK_STREAM, 0);
	std::cout << "Server id = " << socketServer << std::endl;

	int	enable = 1;
	if (setsockopt(socketServer, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
		std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;

	struct sockaddr_in addrServer;
	addrServer.sin_addr.s_addr = inet_addr("127.0.0.1");
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(30000);

	if (bind(socketServer, (const struct sockaddr *)&addrServer, sizeof(addrServer)) == -1)
	{
		std::cout << "bind socketServer issue\n";
		close(socketServer);
		return (EXIT_FAILURE);
	}
	listen(socketServer, 5);

	struct sockaddr_in addrClient;
	socklen_t csize = sizeof(addrClient);

	int	socketClient;
	struct pollfd pfd;
	std::vector<struct pollfd> fds;
	pfd.fd = socketServer;
	pfd.events = POLLIN;
	char buf[19 + 1];
//	int timeout = 5000;
	fds.push_back(pfd);

	while (true)
	{
		if(poll(&fds[0], fds.size(), -1) != -1)	// TODO: add timeout ?
		{
			if (fds[0].revents & POLLIN)
			{
				socketClient = accept(socketServer, (struct sockaddr *)&addrClient, &csize);
				std::cout << "New client " << socketClient << " arrived" << std::endl;
				pfd.fd = socketClient;
				pfd.events = POLLIN | POLLRDHUP | POLLERR;
				fds.push_back(pfd);
				continue ;
			}

			for (std::vector<struct pollfd>::iterator it = fds.begin()++; it != fds.end(); it++)
			{
				if (it->revents == POLLIN)
				{
					bzero(buf, 20);	// TODO: remove forbidden func
					if (recv(it->fd, buf, std::string(buf).length() - 1, 0) <= 0)
						std::cerr << "error recv" << std::endl;
					std::cout << "Client " << it->fd << " says: " << buf;
				}
				else if (it->revents)
				{
					std::cout << "client " << it->fd << " connection closed" << std::endl;
					close(it->fd);
					fds.erase(it);
					break ;
				}
				//if ((fds[0].revents | POLLOUT) == POLLOUT)
				//	send();
			}
		}
	}
	close(socketServer);
	return (EXIT_SUCCESS);
}