#ifndef URL_H
#define URL_H

#include <string>

// static char* KNOWN_PROTOCOLS[6];

class URL
{
public:
	URL(const std::string&);
	URL(const char*);
	~URL();

	std::string toString();
	std::string repr();

	std::string protocol = "";
	std::string username = "";
	std::string password = "";
	std::string host = "";
	std::string path = "";
	std::string anchor = "";
	std::string query_params = "";

	// URL& operator=(const char*&);
};

#endif
