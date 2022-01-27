#include <arpa/inet.h>
#include <iostream>
#include <map>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include "Server.hpp"

Server::Server(void) {
	this->socketServer = socket(AF_INET, SOCK_STREAM, 0);
	std::cout << "Server id = " << this->socketServer << std::endl;

	int	enable = 1;
	if (setsockopt(this->socketServer, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
		std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;
}

Server::Server(Server const& rhs) { *this = rhs; }

Server::~Server(void) {
	close(this->socketServer);
}

Server&	Server::operator=(Server const& rhs) {
	if (this == &rhs)
		return (*this);
	this->socketServer = rhs.socketServer;
	this->fds = rhs.fds;
	this->msg_to_client = rhs.msg_to_client;
	return (*this);
}

int Server::getSocket(void) const {
	return this->socketServer;
}

int Server::initServ(int port) {
	struct sockaddr_in addrServer;
	addrServer.sin_addr.s_addr = inet_addr("127.0.0.1");
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(port);

	if (bind(this->socketServer, (const struct sockaddr *)&addrServer, sizeof(addrServer)) == -1)
	{
		std::cout << "bind socketServer issue\n";
		close(this->socketServer);
		return (false);
	}
	listen(this->socketServer, 5);

	struct pollfd pfd;
	pfd.fd = getSocket();
	pfd.events = POLLIN;
	fds.push_back(pfd);

	return (true);
}

void Server::acceptClient(void) {
	struct pollfd pfd;
	struct sockaddr_in addrClient;
	socklen_t csize = sizeof(addrClient);

	pfd.fd = accept(this->socketServer, (struct sockaddr *)&addrClient, &csize);
	pfd.events = POLLIN | POLLOUT | POLLRDHUP | POLLERR;
	fds.push_back(pfd);

	std::cout << "New client " << pfd.fd << " arrived" << std::endl;
}

void Server::listenRequest(std::vector<struct pollfd>::iterator it) {
	char buf[4096 + 1];
	int read = recv(it->fd, buf, 4096, 0);
	if (read == -1)
		std::cerr << "error recv" << std::endl;
	buf[read] = '\0';
	std::cout << "Client " << it->fd << " says: " << buf;
	msg_to_client[it->fd] = "hell \n";
}

void Server::answerRequest(std::vector<struct pollfd>::iterator it) {
	if (send(it->fd, msg_to_client[it->fd].c_str(), msg_to_client[it->fd].length(), 0) <= 0)
		std::cerr << "send error \n";
	msg_to_client.erase(it->fd);
}

void Server::endConnection(std::vector<struct pollfd>::iterator it)
{
	std::cout << "client " << it->fd << " connection closed" << std::endl;
	close(it->fd);
	msg_to_client.erase(it->fd);
	fds.erase(it);
}

void Server::launch(void)
{
	if (poll(&fds[0], fds.size(), 0) != -1) // TODO: add timeout ?
	{
		if (fds[0].revents & POLLIN)
		{
			this->acceptClient();
			return;
		}

		for (std::vector<struct pollfd>::iterator it = fds.begin()++; it != fds.end(); it++)
		{
			if (it->revents == POLLIN || (it->revents == (POLLIN | POLLOUT))) // le client nous envoie un message
				listenRequest(it);
			else if (it->revents == POLLOUT && msg_to_client.count(it->fd)) // le client est pret a recevoir un message
				answerRequest(it);
			else if (it->revents & POLLERR || it->revents & POLLRDHUP) // le client se deconnecte
			{
				endConnection(it);
				break;
			}
		}
	}
}