#pragma once

#include <Conf.hpp>
#include <vector>
#include <map>
#include <cstring>

class Server
{
	public:
		Server(std::vector<Conf> confs);
		Server(Server const& rhs);
		~Server();

		int initServ(int port);
		void launch();

		int getSocket() const;
		std::vector<Conf> getConfs() const;

		Server& operator=(Server const& rhs);

	private:
		Server();
		
		int	socketServer;
		std::vector<struct pollfd> fds;
		std::map<int, std::string> msg_to_client;
		std::vector<Conf> confs;

		void acceptClient();
		void answerRequest(std::vector<struct pollfd>::iterator);
		void listenRequest(std::vector<struct pollfd>::iterator);
		void endConnection(std::vector<struct pollfd>::iterator);
};