#include "cgi.hpp"
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


bool Server::initServ(int port) {
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
		std::cout << "bind socketServer issue on port: " << port << std::endl;
		return (false);
	}
	if (listen(this->socketServer, 1024) == -1)	// TODO: 1024 ?
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

/* AJOUTER UN NOUVEAU CLIENT A L ATTRIBUT VECTOR<POLLFD> FDS */
void Server::acceptClient(void) {
	struct pollfd pfd;
	struct sockaddr_in addrClient;
	socklen_t csize = sizeof(addrClient);

	pfd.fd = accept(this->socketServer, (struct sockaddr *)&addrClient, &csize);
	pfd.events = POLLIN | POLLOUT | POLLRDHUP | POLLERR;
	fds.push_back(pfd);

	std::cout << "New client " << pfd.fd << " arrived" << std::endl;
}

/*  */
void Server::listenRequest(int client_id) {
	Request & req = msg_from_client[client_id];//define pour simplifier

	std::cout << "server listening for client " << client_id << std::endl;

	char buf[BUF_SIZE + 1];
	int read = recv(client_id, buf, BUF_SIZE, 0);
	if (read == -1)
	{
		std::cerr << "recv error" << std::endl;
		exit (EXIT_FAILURE);
	}
	buf[read] = '\0';

	req.headerSize = Utils::header_is_full(req.buffer);
	if(!req.headerSize)//le header n'est pas pret a etre parse
	{
		req.buffer += buf;
		buf[0] = '\0';
		req.headerSize = Utils::header_is_full(req.buffer);
	}
	if (req.headerSize) // header full
	{
		if (req.getMethod().empty()) //condition pour parser une seule fois
			req = Parser::parseRequest(req);
		std::size_t const client_max_body_size = Utils::selectConf(this->confs, req.getServerName()).getClientMaxBodySize();//trouver max_body_size grace a la conf
		if(client_max_body_size && req.buffer.length() > req.headerSize + client_max_body_size)
			req.statusCode = TOO_LARGE;
		else if (req.buffer.length() < req.headerSize + req.getContentLength()
				&& (!client_max_body_size || (req.buffer.length() < req.headerSize + client_max_body_size)))//si on a pas atteint le maxbodysize ou le content length
			req.buffer += buf;
		//tronque si trop grand
		if (client_max_body_size)
			req.buffer = req.buffer.substr(0, req.headerSize + client_max_body_size);
		req.buffer = req.buffer.substr(0, req.headerSize + req.getContentLength());
	}
}

void Server::answerRequest(int client_id) {
	//	std::cout << "--------\n";
	//	std::cout << "Client " << client_id << " says: \n"
	//			  << msg_from_client[client_id].buffer;
	//	std::cout << "--------\n";

	//	std::cout << "server answering to client " << client_id << std::endl;
	//Request resquest = Parser::parseRequest(msg_from_client[client_id]);// TODO: deja fait dans listen ??
	Response response;
	// SI request.extension == extension CGI
	//if (msg_from_client[client_id].getPath() == "/phpinfo.php")
	//	msg_to_client[client_id] = launchCGI(msg_from_client[client_id]);
	//msg_to_client[client_id] = response.prepareCGI(msg_from_client[client_id], this->confs);
	// SINON
	//else
	std::cout << "Request = |" << std::endl << msg_from_client[client_id] << "|\n";
	msg_to_client[client_id] = response.prepareResponse(msg_from_client[client_id], this->confs);
	
	//	msg_to_client[client_id] = std::string("HTTP/1.1 301 Moved Permanently\nServer: nginx/1.18.0\nDate: Fri, 04 Feb 2022 11:28:19 GMT\nContent-Type: text/html\nContent-Length: 169\nConnection: keep-alive\nLocation: https://google.com/"); // TODO: test redirections
	
	std::cout << "Response: " << msg_to_client[client_id] << std::endl;
	if (send(client_id, msg_to_client[client_id].c_str(), msg_to_client[client_id].length(), 0) <= 0)
		std::cerr << "send error \n"; // g_error ? 
}

void Server::endConnection(std::vector<struct pollfd>::iterator it)
{
	std::cout << "client " << it->fd << " connection closed" << std::endl;
	close(it->fd);
	msg_to_client.erase(it->fd);
	msg_from_client.erase(it->fd);
	fds.erase(it);
}

void Server::launch(void)
{
	if (poll(&fds[0], fds.size(), 0) != -1)
	{
		if (fds[0].revents & POLLIN)/* Un client se connecte */
		{
			this->acceptClient();
			return;
		}

		for (std::vector<struct pollfd>::iterator it = fds.begin() + 1; it != fds.end(); it++)
		{
			if (it->revents == POLLIN || (it->revents == (POLLIN | POLLOUT))) /* le client nous envoie un message */
				listenRequest(it->fd);
			else if (it->revents == POLLOUT && msg_from_client.count(it->fd)) /* le client est pret a recevoir un message */
			{
				answerRequest(it->fd);
				endConnection(it);
				break ;
			}
			else if (it->revents & POLLERR || it->revents & POLLRDHUP) /* le client se deconnecte */
			{
				endConnection(it);
				break ;
			}
		}
	}
}