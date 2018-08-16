#ifndef HTTP
#define HTTP

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 0x2000
#endif

#include <string>
#include <sstream>
#include <vector>

#ifdef DEBUG
#include <iostream>
#endif

extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
}

#include "utils.h"

class HTTPResponse {
public:
	HTTPResponse(const char*);

	void setStatusline(std::string);
	std::string toString();

	std::string statusline;
	std::string protocol;
	unsigned long status;
	std::string reason;
	std::string transfer_encoding;
	std::vector<std::pair<std::string, std::string>> headers;
	std::string body;
};

class HTTPRequest {
public:
	HTTPRequest(const char*);

	void setRequestline(std::string);
	std::string toString();
	HTTPResponse Send(const int&);

	std::string requestline;
	std::string method;
	std::string path;
	std::string protocol;
	std::vector<std::pair<std::string, std::string>> headers;
	std::string body;
};

#endif
