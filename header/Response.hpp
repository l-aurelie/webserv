#pragma once

#include "Conf.hpp"
#include "Request.hpp"
#include <string>
#include <sstream>
#include <vector>

class Response {
	public:
		Response();
		//Response(Request &request);
		Response(Response const& rhs);
		~Response();

		Response &operator=(Response const& rhs);

		std::string prepareResponse(Request & request, std::vector<Conf> &confs);

	private:
		// Header
		std::string protocolVersion;
		std::string statusCode;
		std::size_t contentLength;
		std::string contentType;
		std::string server;

		// Body
		std::string body;

		std::string findFilePath(Request &request, Conf &conf);
		bool selectConf(std::vector<Conf> &confs, Request &request, Conf & ret) const;
		void fillHeader();
		void fillBody(Request & request, Conf conf);
};