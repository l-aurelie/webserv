#pragma once

#include <vector>
#include <map>

class Server
{
	public:
		Server();
		Server(Server const& rhs);
		~Server();

		int initServ(int port);
		int getSocket() const;
		void launch();

		Server& operator=(Server const& rhs);
	
	private:
		int	socketServer;
		std::vector<struct pollfd> fds;
		std::map<int, std::string> msg_to_client;

		void acceptClient();
		void answerRequest(std::vector<struct pollfd>::iterator);
		void listenRequest(std::vector<struct pollfd>::iterator);
		void endConnection(std::vector<struct pollfd>::iterator);
};