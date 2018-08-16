#ifndef HTTP_LOADER
#define HTTP_LOADER

#ifdef DEBUG
#include <iostream>
#endif

#include <exception>
#include <cstdlib>

extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
}

#include "globals.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 0x2000
#endif

static const struct protoent* TCP = getprotobyname("tcp");
static const struct addrinfo hint = {
	0,
	AF_INET,
	SOCK_STREAM,
	TCP->p_proto,
	0,
	nullptr,
	0,
	nullptr
};


class http_loader
{
public:
	int sock;
	bool connected = false;
	std::string url;
	char* buffer;

	http_loader();
	~http_loader();

	Glib::RefPtr< Gio::InputStream > load_url(const litehtml::tstring&);
	const char* get_url() const;
	static std::string getHostFromURL(const char*);
	bool Connect(const std::string&);
	static size_t curlWriteFunction( void *ptr, size_t size, size_t nmemb, void *stream );
};

inline const char* http_loader::get_url() const
{
	return url.c_str();
}

#endif
