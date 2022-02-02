#include "Conf.hpp"
#include "Server.hpp"
#include "Request.hpp"
#include "Parser.hpp"
#include "Response.hpp"
#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

Server::Server(std::vector<Conf> confs) : confs(confs) {}
Server::Server(Server const& rhs) { *this = rhs; }
Server::~Server(void) { close(this->socketServer); }

Server&	Server::operator=(Server const& rhs) {
	if (this == &rhs)
		return (*this);
	this->socketServer = rhs.socketServer;
	this->fds = rhs.fds;
	this->msg_to_client = rhs.msg_to_client;
	this->confs = rhs.confs;
	return (*this);
}

int Server::getSocket() const { return (this->socketServer); }
std::vector<Conf> Server::getConfs() const { return (this->confs); }

//================================================================//


int Server::initServ(int port) {
	std::cout << "New server on " << confs[0].getListen() << std::endl;

	this->socketServer = socket(AF_INET, SOCK_STREAM, 0);

	int	enable = 1;
	if (setsockopt(this->socketServer, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
		std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;

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
	if (listen(this->socketServer, 5) == -1)	// TODO: 5 ?
	{
		std::cerr << "listen socketServer issue\n";
		close(this->socketServer);
		exit(EXIT_FAILURE);// g_error ? 
	}

	struct pollfd pfd;
	pfd.fd = getSocket();
	pfd.events = POLLIN;
	fds.push_back(pfd);

	return (true);
}

/* AJOUTER UN NOUVEAU CLIENT A L ATTRIBUT FDS VECTOR<POLLFD> FDS*/
void Server::acceptClient(void) {
	struct pollfd pfd;
	struct sockaddr_in addrClient;
	socklen_t csize = sizeof(addrClient);

	pfd.fd = accept(this->socketServer, (struct sockaddr *)&addrClient, &csize);
	pfd.events = POLLIN | POLLOUT | POLLRDHUP | POLLERR;
	fds.push_back(pfd);

	std::cout << "New client " << pfd.fd << " arrived" << std::endl;
}

/* RECOIT LA REQUETE CLIENT DANS BUFF, LA PARSE DANS UN OBJET REQUEST, PREPARE LA REPONSE A L'AIDE D'UN OBJET RESPONSE, LA PLACE DANS LA STRING MESSAGE_TO_CLIENT CORRESPONDANT AU FD CLIENT */
void Server::listenRequest(std::vector<struct pollfd>::iterator it) {
	char buf[4096 + 1];//TODO: recuperer taille standard doc RFC
	int read = recv(it->fd, buf, 4096, 0);
	if (read == -1) // g_error
		std::cerr << "error recv" << std::endl;
	buf[read] = '\0';
	std::cout << "--------\n";
	std::cout << "Client " << it->fd << " says: \n" << buf;
	std::cout << "--------\n";
	Request resquest = Parser::parseRequest(buf);
	Response response;
	msg_to_client[it->fd] = response.prepareResponse(resquest, confs); //toutes les confs d'un port
}

/* ENVOIE LA REPONSE CONTENUE DANS LA STRING MESSAGE_TO_CLIENT AU CLIENT */
void Server::answerRequest(std::vector<struct pollfd>::iterator it) {
	std::cout << "Response: " << msg_to_client[it->fd] << std::endl;
	if (send(it->fd, msg_to_client[it->fd].c_str(), msg_to_client[it->fd].length(), 0) <= 0)
		std::cerr << "send error \n"; // g_error ? 
	msg_to_client.erase(it->fd);	// optional if endConnection
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
	/* ACCEPTE NOUVEAUX CLIENTS */
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
			{
				listenRequest(it);
			}
			else if (it->revents == POLLOUT && msg_to_client.count(it->fd)) // le client est pret a recevoir un message
			{
				answerRequest(it);
				endConnection(it);
				break ;
			}
			else if (it->revents & POLLERR || it->revents & POLLRDHUP) // le client se deconnecte
			{
				endConnection(it);
				break;
			}
		}
	}
}