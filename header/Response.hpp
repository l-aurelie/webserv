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
		Response(Response const& rhs);
		~Response();

		Response &operator=(Response const& rhs);

		std::fstream tmpFile;
		std::string tmpFilename;

		void prepareResponse(Request & request, std::vector<Conf> &confs);

	private:
		std::string path;
		std::string queryString;
		std::string protocolVersion;
		std::string statusCode;
		off_t contentLength;
		std::string server;
		std::string location;
		std::string date;
		std::string lastModified;

		void createTMPFile();
		void fillErrorBody(std::string code, Conf & conf);
		int fillHeader();

		int emptyHeadersInFile();
		void constructPath(Request &request, Conf const& conf);
		void redirected(int code, std::string const& url);
		void setContentType();
		void setContentLength(int headerSize);
		std::string matchingExtensionType(const std::string &extension);
		void autoIndex(std::string const& path, std::string const& root);
		void methodDelete(Request &request, Conf const &conf);

	public:
		std::string sendBuf;
		int sendLength;
	//	std::string body;
		std::ifstream bodyStream;
		std::string contentType;

		std::string const& getPath() const;
		std::string const& getQueryString() const;

		void error(std::string const & status_code, std::string const & error);
};
