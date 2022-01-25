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
	std::map<int, std::string> msg_to_client;
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
				pfd.events = POLLIN | POLLOUT | POLLRDHUP | POLLERR;
				fds.push_back(pfd);
				continue ;
			}

			for (std::vector<struct pollfd>::iterator it = fds.begin()++; it != fds.end(); it++)
			{
				if (it->revents & POLLIN)
					std::cout << "revents = POLLIN" << std::endl;
				if (it->revents == POLLOUT && msg_to_client.count(it->fd))
					std::cout << "revents = POLLOUT" << std::endl;
				if (it->revents & POLLERR)
					std::cout << "revents = POLLERR" << std::endl;
				if (it->revents & POLLRDHUP)
					std::cout << "revents = POLLRDHUP" << std::endl;

				if (it->revents == POLLIN || (it->revents == (POLLIN | POLLOUT)))
				{
					bzero(buf, 20);	// TODO: remove forbidden func
					std::cout << "waiting for recv" << std::endl;
					if (recv(it->fd, buf, std::string(buf).length() - 1, 0) <= 0)
						std::cerr << "error recv" << std::endl;
					std::cout << "Client " << it->fd << " says: " << buf;
					msg_to_client[it->fd] = "hell \n";
				}
				else if (it->revents == POLLOUT && msg_to_client.count(it->fd)) {
					std::cout << "waiting for send" << std::endl;
					if (send(it->fd, msg_to_client[it->fd].c_str(), msg_to_client[it->fd].length(), 0) <= 0)
						std::cerr << "send error \n";
					msg_to_client.erase(it->fd);
				}
				else if (it->revents & POLLERR || it->revents & POLLRDHUP)
				{
					std::cout << "erreur revents = " << it->revents << std::endl;
					std::cout << "client " << it->fd << " connection closed" << std::endl;
					msg_to_client.erase(it->fd);
					close(it->fd);
					fds.erase(it);
					break ;
				}
			}
		}
	}
	close(socketServer);
	return (EXIT_SUCCESS);
}