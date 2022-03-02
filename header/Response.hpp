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
		std::string sendBuf;
		int sendLength;
		std::ifstream bodyStream;
		std::string contentType;

		std::string const& getPath() const;
		std::string const& getQueryString() const;
		void error(std::string const & status_code, std::string const & error);
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

		void methodDelete(Request &request, Conf const &conf);
		void autoIndex(std::string const& path, std::string const& root);
		void redirected(int code, std::string const& url);
		void createTMPFile();
		void constructPath(Request &request, Conf const& conf);
		int emptyHeadersInFile();
		void setContentType();
		void setContentLength(int headerSize);
		void prepareBody(Request & request, Conf &conf);
		void fillErrorBody(std::string code, Conf & conf);
		int fillHeader();
		void firstFillSendBuf(int header_len);
};
