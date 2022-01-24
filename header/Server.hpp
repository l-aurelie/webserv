#pragma once

class Server
{
	public:
		Server(void);
		Server(Server const& rhs);
		~Server(void);

		Server& operator=(Server const& rhs);
};