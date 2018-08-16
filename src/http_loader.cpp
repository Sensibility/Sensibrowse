#include "http_loader.h"
#include "url.h"
#include "http.h"

http_loader::http_loader()
{
	sock = socket(AF_INET, SOCK_STREAM, TCP->p_proto);
	buffer = new char[BUFFER_SIZE];
}

http_loader::~http_loader()
{
	close(sock);
	delete[] buffer;
}

size_t http_loader::curlWriteFunction( void *ptr, size_t size, size_t nmemb, void *stream )
{
	Glib::RefPtr< Gio::MemoryInputStream >* s = (Glib::RefPtr< Gio::MemoryInputStream >*) stream;
	(*s)->add_data(ptr, size * nmemb);
	return size * nmemb;
}

bool http_loader::Connect(const std::string& host) {
	// Attempting to (re)connect to the host specified by host
	this->connected = false;
	struct addrinfo* addrs;
	struct addrinfo* addr;
	int addrinfoResponse = getaddrinfo(host.c_str(), "80", &hint, &addrs);
	if ( addrinfoResponse || addrs == nullptr) {
		std::cerr << "Error fetching addrinfo for '" << host << '\'' << ": Host not known" << std::endl;
		#ifdef DEBUG
		std::cerr << "Error: " << gai_strerror(addrinfoResponse) << std::endl;
		#endif
		return false;
	}

	for (addr = addrs; addr != nullptr; addr = addr->ai_next) {
		if (addr->ai_addrlen > 0 && addr->ai_family == AF_INET) {
			break;
		}
	}

	if (addr == nullptr) {
		std::cerr << "Error making connection: Could not resolve '" << host << "' to IPv4 Address" << std::endl;
		return false;
	}

	#ifdef DEBUG
	std::cout << "Got the addrinfo" << std::endl << "Conecting..." << std::endl << std::endl;
	#endif

	if (connect(this->sock, addr->ai_addr, addr->ai_addrlen)) {
		std::cerr << "Error making connection to: " << host << std::endl;
		return false;
	}
	this->connected = true;
	freeaddrinfo(addrs);
	return true;
}

Glib::RefPtr< Gio::InputStream > http_loader::load_url(const litehtml::tstring& url)
{
	this->url = url;

	URL parsedURL=url;

	#if DEBUG
	std::cout << "Fetching via GET: " << url << " - " << parsedURL.repr() << std::endl;
	#endif

	Glib::RefPtr< Gio::MemoryInputStream > stream = Gio::MemoryInputStream::create();

	if (!this->connected) {
		if(!this->Connect(parsedURL.host)) {
			std::cerr << "Connection failed." << std::endl;
			return stream;
		}
	#ifdef DEBUG
		std::cout << "Connected." << std::endl;
	}
	else {
		std::cout << "Using existing connection..." << std::endl;
	#endif
	}


	std::string request = "GET " + parsedURL.path + " HTTP/1.1\r\nConnection: Keep-Alive\r\nAccept-Encoding: identity\r\nAccept: text/html,text/plain;q=0.9,text/*;q=0.8,*/*;q=0.7\r\nAccept-Charset: utf-8\r\nAccept-Language: en-US\r\nDNT: 1\r\nUser-Agent: sensibrowse/0.1\r\nScript: paradisi,wasm,none;q=0.9,javascript;q=0.8\r\nHost: " + parsedURL.host + "\r\n\r\n";
	std::cout << request;

	size_t offset=0,  dataSize=0;
	do {
		dataSize = send(this->sock, request.c_str() + offset, request.length() - offset, 0);
		#ifdef DEBUG
		std::cout << "Sent " << dataSize << " bytes of data to " << this->url << '.' << std::endl;
		#endif

		offset += dataSize > 0 ? dataSize : 0;
		if (dataSize <= 0 ) {
			#ifdef DEBUG
			std::cout << "Disconnect detected; attempting reconnect" << std::endl;
			#endif

			if (!this->Connect(parsedURL.host)) {
				return stream;
			}
		}
	} while(offset < request.length());


	std::string response = "";

	do {
		std::memset(this->buffer, '\0', BUFFER_SIZE);
		dataSize = recv(this->sock, this->buffer, BUFFER_SIZE, 0);

		if (dataSize < 0) {
			#ifdef DEBUG
			std::cout << "Disconnect detected; attempting reconnect" << std::endl;
			#endif

			if (!this->Connect(parsedURL.host)) {
				return stream;
			}

			dataSize = BUFFER_SIZE;

		}
		else {
			response += this->buffer;

			#ifdef DEBUG
			std::cout << "Recieved " << dataSize << " bytes from " << this->url << '.' << std::endl;
			#endif
		}

	} while (dataSize == BUFFER_SIZE);

	HTTPResponse resp = response.c_str();
	response.clear();

	if (resp.transfer_encoding == "chunked" or resp.transfer_encoding == "Chunked") {
		#ifdef DEBUG
		std::cout << "Chunked encoding, fetching and parsing chunks" << std::endl;
		#endif

		std::stringstream ss;
		response = resp.body;

		// First we should parse the chunk we already have
		resp.body.clear();
		unsigned int chunklen = 0;

		size_t pos = response.find("\r\n");
		ss << std::hex << response.substr(0, pos);
		ss >> chunklen;
		std::string chunk = response.substr(pos+2, chunklen);

		#ifdef DEBUG
		std::cout << "===============   Chunk   ===============" << std::endl << chunk << std::endl << "================   End   ================" << std::endl;
		#endif

		resp.body = chunk;

		response = response.substr(pos+4+chunklen);

		unsigned int CHUNKLEN = 0;
		do {

			do {
				std::memset(this->buffer, '\0', BUFFER_SIZE);
				dataSize = recv(this->sock, this->buffer, BUFFER_SIZE, 0);

				#ifdef DEBUG
				std::cout << "Recieved " << dataSize << " bytes from " << this->url << std::endl;
				#endif

				response += this->buffer;
			} while (dataSize == BUFFER_SIZE);

			pos = response.find("\r\n");

			if (pos == std::string::npos) {
				break;
			}

			std::string tmp = response.substr(0, pos);

			// The only way to clear the contents of a stringstream is to make a new stringstream...
			std::stringstream s;
			s << std::hex << tmp;
			s >> CHUNKLEN;

			#ifdef DEBUG
			std::cout << "Chunk length: " << CHUNKLEN << 'B' << std::endl;
			if (CHUNKLEN == 0) {
				std::cout << '\'' << tmp << '\'' << std::endl;
			}
			#endif

			chunk = response.substr(pos+2, CHUNKLEN);

			#ifdef DEBUG
			std::cout << "===============   Chunk   ===============" << std::endl << chunk << "================   End   ================" << std::endl;
			#endif

			resp.body += chunk;

			response = response.substr(pos+4+CHUNKLEN);

			#ifdef DEBUG
			if (!response.empty()) {
				std::cout << "Remaining response text after chunk parse: '" << response << '\'' << std::endl;
			}
			#endif

		} while (CHUNKLEN > 0);
	}

	std::cout << resp.toString() << std::endl;

	stream->add_data(resp.body);

	return stream;
}

std::string http_loader::getHostFromURL(const char* url) {
	std::string ret = url;
	size_t pos = ret.find("://");
	if (pos == std::string::npos) {
		std::cerr << '\'' << url << "' does not appear to be a URL!" << std::endl;
		return std::string("");
	}

	ret = ret.substr(pos+3);

	pos = ret.find("/");
	if (pos != std::string::npos) {
		ret = ret.substr(0, pos);
	}

	std::cout << "Host for '" << url << "' appears to be " << ret << std::endl;

	return ret;
}
