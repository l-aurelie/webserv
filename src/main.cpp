#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <string>
#include <unistd.h>
#include "Server.hpp"
#include <poll.h>

#define CLOSED -1

//struct pollfd *enlarge(struct)
{
	struct pollfd *fds_new;
}

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
	int nb_clients = 1;
	int max_clients = 5;
	struct pollfd fds[5];
	//struct pollfd *fds;
	//fds = new fds[max_clients];
	for (int i = 0; i < max_clients; i++) fds[i].fd = -1;
	fds[0].fd = socketServer;
	fds[0].events = POLLIN;
	char buf[19 + 1];
//	int timeout = 5000;

	while (1)
	{
		//if(nb_client + 1 == max_clients)
		//{
			//enlarge_fds();
			//continue;

	//	}
		if(poll(fds, nb_clients, -1) != -1)
		{
			if (fds[0].revents & POLLIN)
			{
				socketClient = accept(socketServer, (struct sockaddr *)&addrClient, &csize);
				if (nb_clients < max_clients)	// TODO: augmenter le tableau
				{
					std::cout << "New client " << socketClient << " arrived" << std::endl;
					fds[nb_clients].fd = socketClient;
					fds[nb_clients].events = POLLIN | POLLRDHUP | POLLERR;
					nb_clients++;
					continue ;
				}
				else
					std::cerr << "max buffer size reached: cannot accept more clients" << std::endl;
			}

			for (int i = 1; i < nb_clients; i++)
			{
				if (fds[i].fd != CLOSED && fds[i].revents == POLLIN)
				{
					bzero(buf, 20);	// TODO: remove forbidden func
					if (recv(fds[i].fd, buf, std::string(buf).length() - 1, 0) <= 0)
						std::cerr << "error recv" << std::endl;
					std::cout << "Client " << fds[i].fd << " says: " << buf;
				}
				else if (fds[i].fd != CLOSED && fds[i].revents)
				{
					std::cout << "client " << fds[i].fd << " connection closed" << std::endl;
					close(fds[i].fd);
					fds[i].fd = CLOSED;
					fds[i].events = 0;
					fds[i].revents = 0;
				}
				//if ((fds[0].revents | POLLOUT) == POLLOUT)
				//	send();
			}
		}
	}
	close(socketServer);
	return (EXIT_SUCCESS);
}