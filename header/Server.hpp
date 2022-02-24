#pragma once

#include "Conf.hpp"
#include "Request.hpp"
#include <vector>
#include <map>
#include <cstring>

class Server
{
	public:
		Server(std::vector<Conf> confs);
		Server(Server const& rhs);
		~Server();

		bool initServ(int port);
		void launch();

		int getSocket() const;
		std::vector<Conf> getConfs() const;

		Server& operator=(Server const& rhs);

	private:
		Server();
		
		int	socketServer;
		std::vector<struct pollfd> fds;
		std::map<int, Request> msg_from_client;
		std::map<int, std::string> msg_to_client;
		std::vector<Conf> confs;

		void acceptClient();
		void answerRequest(std::vector<struct pollfd>::iterator it);
		void listenRequest(std::vector<struct pollfd>::iterator it);
		void endConnection(std::vector<struct pollfd>::iterator it);
		void findHeaderSizeAndContentLength(std::string const& buf, std::size_t & header_size, std::size_t & content_length) const;



};
