#include "Conf.hpp"
#include "Server.hpp"
#include "Request.hpp"
#include "Parser.hpp"
#include "Response.hpp"
#include "ServerUtils.hpp"
#include "Utils.hpp"
#include "webserv.hpp"

#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

Server::Server(std::vector<Conf> confs) : confs(confs) {}
Server::Server(Server const& rhs) { *this = rhs; }
Server::~Server(void) { close(this->socketServer); }

Server&	Server::operator=(Server const& rhs)
{
	if (this == &rhs)
		return (*this);
	this->socketServer = rhs.socketServer;
	this->fds = rhs.fds;
	this->msg_from_client = rhs.msg_from_client;
	this->msg_to_client = rhs.msg_to_client;
	this->confs = rhs.confs;
	return (*this);
}

int Server::getSocket() const { return (this->socketServer); }
std::vector<Conf> Server::getConfs() const { return (this->confs); }

//================================================================//

/* CREATION SOCKET SERVER, BIND, MISE EN ECOUTE AJOUT SOCKETSERV COMME PREMIER ELEMENT DU VECT<POLLFD> */
bool Server::initServ(int port)
{
	std::cout << "New server on " << confs[0].listen << std::endl;

	this->socketServer = socket(AF_INET, SOCK_STREAM, 0);
	if (this->socketServer == -1)
	{
		std::cerr << "socket syscall failed\n";
		exit(EXIT_FAILURE);
	}

	int	enable = 1;
	if (setsockopt(this->socketServer, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1)
		std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;

	struct sockaddr_in addrServer;
	addrServer.sin_addr.s_addr = inet_addr("127.0.0.1");
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(port);

	if (bind(this->socketServer, (const struct sockaddr *)&addrServer, sizeof(addrServer)) == -1)
	{
		std::cout << "bind socketServer issue on port: " << port << std::endl;
		return (false);
	}
	if (listen(this->socketServer, BUF_SIZE) == -1)
	{
		std::cerr << "listen socketServer issue\n";
		close(this->socketServer);
		exit(EXIT_FAILURE);
	}

	//-- Ajoute le socketserver comme premier element a lattribut vector<pollfd> fds
	struct pollfd pfd;
	pfd.fd = getSocket();
	pfd.events = POLLIN;
	fds.push_back(pfd);
	return (true);
}

/* AJOUTER UN NOUVEAU CLIENT A L ATTRIBUT VECTOR<POLLFD> FDS */
void Server::acceptClient(void)
{
	struct pollfd pfd;
	struct sockaddr_in addrClient;
	socklen_t csize = sizeof(addrClient);

	pfd.fd = accept(this->socketServer, (struct sockaddr *)&addrClient, &csize);
	if (pfd.fd == -1)
	{
		std::cerr << "accept syscall failed\n";
		return ;
	}
	//pfd.events = POLLIN | POLLOUT | POLLRDHUP | POLLERR; // TODO: not on mac
	pfd.events = POLLIN | POLLOUT | POLLERR;
	fds.push_back(pfd);
}

/* PREPARATION DE L'OBJET REQUEST: TANT QUE LE CLIENT EST EN POLLIN : SON MESSAGE N'EST PAS TERMINE:
 ON CONTINUE DE RECV ET DE CONCATENER. Header placé dans headerBuf, body dans tmpFile */
void Server::listenRequest(std::vector<struct pollfd>::iterator it)
{
	int client_id = it->fd;
	Request & req = msg_from_client[client_id];//define pour simplifier
	if (req.tmpFile.fail())
	{
		std::cerr << "failed to open tmpfile" << std::endl;
		endConnection(it);
		return ;
	}
	//-- On  lit dans buf ce que le client nous envoie, la taille lue dans read
	char buf[BUF_SIZE + 1];
	int bytes_read = recv(client_id, buf, BUF_SIZE, 0);
	if (bytes_read <= 0)
	{
		std::cerr << "recv error" << std::endl;
		endConnection(it);
		return ;
	}
	buf[bytes_read] = '\0';
	//-- Rempli header et body de l'objet request
	ServerUtils::parseRecv(bytes_read, buf, req, this->confs);
}

/* ON PREPARE  ET ENVOIE LA REPONSE  AU CLIENT (on sait que le client attend une reponse :
 poll POLLOUT et il est present dans la map msg_from_client car a effectue une requete) */
void Server::answerRequest(std::vector<struct pollfd>::iterator it)
{
	int client_id = it->fd;
	Response response;
	msg_to_client[client_id] = response.prepareResponse(msg_from_client[client_id], this->confs);
	if (send(client_id, msg_to_client[client_id].c_str(), msg_to_client[client_id].length(), 0) <= 0)
	{
		std::cerr << "send error \n";
		endConnection(it);
	}
}

/* GERE DECONNECTION, CLOSE FD, SUPPRIME REQUETE ET REPONSE CORRESPONDANTE */
void Server::endConnection(std::vector<struct pollfd>::iterator it)
{
	close(it->fd);
	msg_to_client.erase(it->fd);
	msg_from_client.erase(it->fd);
	fds.erase(it);
}

/* 1 SERVER ECOUTE LES DEMANDES DE CONNECTION, LES REQUETES ET ENVOIE LES REPONSES AUX PRECEDENTES REQUETES */
void Server::launch(void)
{
	if (poll(&fds[0], fds.size(), 0) != -1)
	{
		//-- Un client se connecte
		if (fds[0].revents & POLLIN)
		{
			this->acceptClient();
			return;
		}
		for (std::vector<struct pollfd>::iterator it = fds.begin() + 1; it != fds.end(); it++)
		{
			//-- un client nous envoie un message
			if (it->revents == POLLIN || (it->revents == (POLLIN | POLLOUT)))
				listenRequest(it);
			//-- un client est pret a recevoir un message
			else if (it->revents == POLLOUT && msg_from_client.count(it->fd))
			{
				answerRequest(it);
				endConnection(it);
				break ;
			}
			//-- le client se deconnecte
			else if (it->revents & POLLERR)
			//else if (it->revents & POLLERR || it->revents & POLLRDHUP) // TODO: do not work on mac
			{
				endConnection(it);
				break ;
			}
		}
	}
}
