#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <string>
#include <unistd.h>
#include "Server.hpp"
#include <poll.h>

int main(void) {
	Server server;

	int socketServer = socket(AF_INET, SOCK_STREAM, 0);

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
	fds[0].fd = socketServer;
	fds[0].events = POLLIN;
	char buf[19 + 1];
//	int timeout = 5000;

	while (1)
	{
		if(poll(fds, nb_clients + 1, -1) != -1)
		{
            if ((fds[0].revents & POLLIN))
			{
                socketClient = accept(socketServer, (struct sockaddr *)&addrClient, &csize);
                std::cout << "New client arrive" << std::endl;
				if (nb_clients < max_clients)	// TODO: augmenter le tableau
				{
					fds[nb_clients].fd = socketClient;
					fds[nb_clients].events = POLLIN | POLLRDHUP | POLLERR;
					nb_clients++;
					continue ;
				}
            }

            for(int i = 1; i < nb_clients; i++)
            {
				if (fds[i].revents & POLLIN)
				{
					bzero(buf, 20);
					if (recv(fds[i].fd, buf, std::string(buf).length() - 1, 0) <= 0)
						std::cerr << "error recv" << std::endl;
					std::cout << "buf: " << buf << '\n';
				}
			//	else if (fds[i].revents & (POLLRDHUP | POLLERR))
			//	{
			//		//close(fds[i].fd);
			//		//fds[i].fd = -1;
			//		std::cout << "connection closed" << std::endl;
			//	}
				//if ((fds[0].revents | POLLOUT) == POLLOUT)
				//	send();
            }
		}
	}

	close(socketClient);
	close(socketServer);

	return (EXIT_SUCCESS);
}