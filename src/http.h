#ifndef HTTP
#define HTTP

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 0x2000
#endif

#include <string>
#include <cstring>
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
	HTTPResponse(char*, const size_t&);
	HTTPResponse(const std::string&);

	void setStatusline(std::string);
	void add_body(void*, const size_t&);
	std::string toString();

	std::string statusline;
	std::string protocol;
	unsigned long status;
	std::string reason;
	std::string transfer_encoding;
	size_t content_length = 0;
	size_t headerlen;
	std::pair<std::string, std::string> content_type;

	// TODO: this should be a map/dict, not a list of pairs
	std::vector<std::pair<std::string, std::string>> headers;

	void* body = nullptr;
	std::string Body();
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
