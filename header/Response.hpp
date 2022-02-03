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
		std::string path;
		std::string protocolVersion;
		std::string statusCode;
		std::size_t contentLength;
		std::string contentType;
		std::string server;

		// Body
		std::string body;

		void findFilePath(Request &request, Conf const& conf);
		void fillHeader();
		void fillBody(Request & request, Conf const& conf);
		std::string format() const;
		void setContentType();
		std::string matchingExtensionType(const std::string &extension);
		std::string errorFillResponse(std::string code);
};