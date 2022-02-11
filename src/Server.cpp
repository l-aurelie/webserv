#include "Conf.hpp"
#include "Server.hpp"
#include "Request.hpp"
#include "Parser.hpp"
#include "Response.hpp"
#include "Utils.hpp"
#include "webserv.hpp"

#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <map>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

std::string launchCGI(); // TODO: remove

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

/* CREATION SOCKET SERVER, BIND, MISE EN ECOUTE AJOUT SOCKETSERV COMME PREMIER ELEMENT DU VECT<POLLFD>*/
bool Server::initServ(int port) {
	std::cout << "New server on " << confs[0].listen << std::endl;

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
		std::cout << "bind socketServer issue on port: " << port << std::endl;
		return (false);
	}
	if (listen(this->socketServer, 1024) == -1)	// TODO: 1024 ?
	{
		std::cerr << "listen socketServer issue\n";
		close(this->socketServer);
		exit(EXIT_FAILURE);
	}
	/* Ajoute le socketserver comme premier element a lattribut vector<pollfd> fds */
	struct pollfd pfd;
	pfd.fd = getSocket();
	pfd.events = POLLIN;
	fds.push_back(pfd);

	return (true);
}

/* AJOUTER UN NOUVEAU CLIENT A L ATTRIBUT VECTOR<POLLFD> FDS */
void Server::acceptClient(void) {
	struct pollfd pfd;
	struct sockaddr_in addrClient;
	socklen_t csize = sizeof(addrClient);

	pfd.fd = accept(this->socketServer, (struct sockaddr *)&addrClient, &csize);
	//pfd.events = POLLIN | POLLOUT | POLLRDHUP | POLLERR; // TODO: not on mac
	pfd.events = POLLIN | POLLOUT | POLLERR;
	fds.push_back(pfd);

	std::cout << "New client " << pfd.fd << " arrived" << std::endl;
}

/* PREPARATION DE L'OBJET REQUEST: TANT QUE LE CLIENT EST EN POLLIN : SON MESSAGE N'EST PAS TERMINE. ON CONTINUE DE RECV ET DE CONCATENER AVEC NOTRE STRING (attribut de requete presente dans la map msg_from_client). LA TAILLE MAXIMUM (maxBodySize/contentLength) EST PRISE EN COMPTE */
void Server::listenRequest(int client_id) {
	Request & req = msg_from_client[client_id];//define pour simplifier

	std::cout << "server listening for client " << client_id << std::endl;
	/* On  lit dans buf ce que le client nous envoie */
	char buf[BUF_SIZE + 1];
	int read = recv(client_id, buf, BUF_SIZE, 0);
	if (read == -1)
	{
		std::cerr << "recv error" << std::endl;
		exit (EXIT_FAILURE);
	}
	buf[read] = '\0';

	/* Tant que le header n'est pas complet on ajoute buf a notre string de requete */
	req.headerSize = Utils::header_is_full(req.buffer);
	if(!req.headerSize)//le header n'est pas pret a etre parse
	{
		req.buffer += buf;
		buf[0] = '\0';
		req.headerSize = Utils::header_is_full(req.buffer);
	}

	/* Le header est complet on parse le  header de la requete */
	if (req.headerSize) // header full
	{
		if (req.getMethod().empty()) //condition pour parser une seule fois
			req = Parser::parseRequest(req);
		std::size_t const client_max_body_size = Utils::selectConf(this->confs, req.getServerName(), req.getPath()).clientMaxBodySize;//trouver max_body_size grace a la conf

		/*
  		std::size_t const client_max_body_size = Utils::selectConf(this->confs, req.getServerName()).getClientMaxBodySize();//trouver max_body_size grace a la conf
		*/
		/* On ajoute buf a notre string de requete tant qu'on a pas atteint content length ou maxbody */
		if(client_max_body_size && req.buffer.length() > req.headerSize + client_max_body_size)
			req.statusCode = TOO_LARGE;
		else if (req.buffer.length() < req.headerSize + req.getContentLength()
				&& (!client_max_body_size || (req.buffer.length() < req.headerSize + client_max_body_size)))//si on a pas atteint le maxbodysize ou le content length
			req.buffer += buf;

		/* Si besoin tronque la string a la taille exacte */
		if (client_max_body_size)
			req.buffer = req.buffer.substr(0, req.headerSize + client_max_body_size);
		req.buffer = req.buffer.substr(0, req.headerSize + req.getContentLength());
		req.setBody();
	}
}

/*  ON PREPARE  ET ENVOIE LA REPONSE  AU CLIENT (on sait que le client attend une reponse : poll POLLOUT et il est present dans la map msg_from_client car a effectue une requete) */ 
void Server::answerRequest(int client_id) {
	if (msg_from_client[client_id].getPath() == "/favicon.ico")	// TODO: disable favicon.ico
	{
		msg_to_client[client_id] = "HTTP/1.1 404 Not Found\nServer: webserv\nContent-Length: 158\nContent-Type: text/html\n\n<!DOCTYPE html>\n<head>\n<title>404 Not Found</title>\n</head>\n<body>\n<center>\n<h1>404 Not Found</h1>\n<hr />\n<h3>webserv</h3>\n</center>\n</body>\n</html>";
		if (send(client_id, msg_to_client[client_id].c_str(), msg_to_client[client_id].length(), 0) <= 0)
			std::cerr << "send error \n"; // g_error ? 
		return ;
	}

	Response response;
	/* Prepare la reponse */
	//std::cout << "Request buf = |" << std::endl << msg_from_client[client_id].buffer << "|\n";
	std::cout << "Request = |" << std::endl << msg_from_client[client_id] << "|\n";
	msg_to_client[client_id] = response.prepareResponse(msg_from_client[client_id], this->confs);
	
	//	msg_to_client[client_id] = std::string("HTTP/1.1 301 Moved Permanently\nServer: nginx/1.18.0\nDate: Fri, 04 Feb 2022 11:28:19 GMT\nContent-Type: text/html\nContent-Length: 169\nConnection: keep-alive\nLocation: https://google.com/"); // TODO: test redirections
	
	/* Envoie la reponse au client */
	std::cout << "Response: " << msg_to_client[client_id] << std::endl;
	if (send(client_id, msg_to_client[client_id].c_str(), msg_to_client[client_id].length(), 0) <= 0)
		std::cerr << "send error \n"; // g_error ? 
}

/* GERE DECONNECTION, CLOSE FD, SUPPRIME REQUETE ET REPONSE CORRESPONDANTE */
void Server::endConnection(std::vector<struct pollfd>::iterator it)
{
	std::cout << "client " << it->fd << " connection closed" << std::endl;
	close(it->fd);
	msg_to_client.erase(it->fd);
	msg_from_client.erase(it->fd);
	fds.erase(it);
}

/* 1 SERVER ECOUTE LES DEMANDES DE CONNECTION, LES REQUETES ET ENVOIE LES REPONSE AU PREVIOUS REQUETES*/
void Server::launch(void)
{
//	std::cout << "acceded in Server::launch\n";
//	launchCGI();
//	exit(12);

	if (poll(&fds[0], fds.size(), 0) != -1)
	{
		/* Un client se connecte */
		if (fds[0].revents & POLLIN)
		{
			this->acceptClient();
			return;
		}

		for (std::vector<struct pollfd>::iterator it = fds.begin() + 1; it != fds.end(); it++)
		{ 
			/* le client nous envoie un message */
			if (it->revents == POLLIN || (it->revents == (POLLIN | POLLOUT)))
				listenRequest(it->fd);

			/* le client est pret a recevoir un message */
			else if (it->revents == POLLOUT && msg_from_client.count(it->fd))
			{
				answerRequest(it->fd);
				endConnection(it);
				break ;
			}
			
			/* le client se deconnecte */
			//else if (it->revents & POLLERR || it->revents & POLLRDHUP)
			else if (it->revents & POLLERR)
			{
				endConnection(it);
				break ;
			}
		}
	}
}
