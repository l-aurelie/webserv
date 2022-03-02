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
	msg_from_client[pfd.fd].createTMPFile();
	//pfd.events = POLLIN | POLLOUT | POLLRDHUP | POLLERR; // TODO: not on mac
	pfd.events = POLLIN | POLLOUT | POLLERR;
	fds.push_back(pfd);
}

/* PREPARATION DE L'OBJET REQUEST: TANT QUE LE CLIENT EST EN POLLIN, SON MESSAGE N'EST PAS TERMINE:
 ON CONTINUE DE RECV */
void Server::listenRequest(std::vector<struct pollfd>::iterator & it)
{
	int client_id = it->fd;
	//-- On  lit dans buf ce que le client nous envoie, la taille lue dans read
	msg_from_client[client_id].bufLength = recv(client_id, msg_from_client[client_id].buf, BUF_SIZE, 0);
	if (msg_from_client[client_id].bufLength <= 0)
	{
		std::cerr << "recv error" << std::endl;
		endConnection(it);
		return ;
	}
	msg_from_client[client_id].buf[msg_from_client[client_id].bufLength] = '\0';
}

/* TRAITE LE MESSAGE RECU DANS BUF DANS UN OBJET REQUEST, Header placé dans headerBuf, body dans tmpFile */
void Server::treatRequest(std::vector<struct pollfd>::iterator & it)
{
	if (msg_from_client[it->fd].tmpFile.fail())
	{
		std::cerr << "failed to open tmpfile '" << msg_from_client[it->fd].tmpFilename << "'" << std::endl;
		endConnection(it);
		return ;
	}
	//-- Rempli header et body de l'objet request
	ServerUtils::parseRecv(msg_from_client[it->fd].bufLength, msg_from_client[it->fd].buf, msg_from_client[it->fd], this->confs);
	msg_from_client[it->fd].bufLength = 0;
}

/* ON PREPARE  ET ENVOIE LA REPONSE  AU CLIENT (on sait que le client attend une reponse :
 poll POLLOUT et il est present dans la map msg_from_client car a effectue une requete) */
bool Server::answerRequest(std::vector<struct pollfd>::iterator & it)
{
	int client_id = it->fd;
	if (msg_to_client[client_id].sendLength == 0)
	{
		msg_to_client[client_id].prepareResponse(msg_from_client[client_id], this->confs);
	}
	else
	{
		//std::cerr << "sending for client_id: " << client_id << std::endl;
		if (send(client_id, msg_to_client[client_id].sendBuf.c_str(), msg_to_client[client_id].sendLength, 0) <= 0)
		{
			std::cerr << "send error" << std::endl;
			endConnection(it);
		}
		msg_to_client[client_id].sendBuf = std::string(4096, '\0');
		msg_to_client[client_id].sendLength = 0;
		if (msg_to_client[client_id].bodyStream.eof())
			return (true);
	}
	return (false);
}

/* GERE DECONNECTION, CLOSE FD, SUPPRIME REQUETE ET REPONSE CORRESPONDANTE */
void Server::endConnection(std::vector<struct pollfd>::iterator & it)
{
	//std::cerr << "closing client " << it->fd << std::endl;
	close(it->fd);
	remove(msg_from_client[it->fd].tmpFilename.c_str());
	remove(msg_to_client[it->fd].tmpFilename.c_str());
	msg_to_client.erase(it->fd);
	msg_from_client.erase(it->fd);
	it = fds.erase(it);
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
		std::vector<struct pollfd>::iterator it = fds.begin() + 1;
		while (it != fds.end())
		{
			//-- Un client nous envoie un message
			if (msg_from_client[it->fd].bufLength)
				treatRequest(it);
			else if (it->revents == POLLIN || (it->revents == (POLLIN | POLLOUT)))
				listenRequest(it);
			//-- Un client est pret a recevoir un message
			else if (it->revents == POLLOUT && msg_from_client.count(it->fd))
			{
				if (answerRequest(it))
				{
					endConnection(it);
					continue ;
				}
			}
			//-- le client se deconnecte
			else if (it->revents & POLLERR)
			//else if (it->revents & POLLERR || it->revents & POLLRDHUP) // TODO: do not work on mac
			{
				endConnection(it);
				continue ;
			}
			++it;
		}
	}
}
