#pragma once

#include "Conf.hpp"
#include "Request.hpp"
#include <map>
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
		std::string queryString;
		std::string protocolVersion;
		std::string statusCode;
		std::size_t contentLength;
		std::string contentType;
		std::string server;
		std::string location;

		std::string body;

		void constructPath(Request &request, Conf const& conf);
		void fillHeader();
		void redirected(int code, std::string const& url);
		void fillBody(Request & request, Conf const& conf);
		std::string format() const;
		void setContentType();
		std::string matchingExtensionType(const std::string &extension);
		std::string errorFillResponse(std::string code, Conf & conf);
		
		void autoIndex(std::string const& path, std::string const& root);
		void deleteFile(Request &request, Conf const &conf);
		void getFile(Request & request, Conf const& conf);
		void launchCGI(Request &request);
		void error(std::string const & status_code, std::string const & error);
};
