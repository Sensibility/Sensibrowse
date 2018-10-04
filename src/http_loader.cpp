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

// Attempts to (re)connect to the host specified by `host
// Returns a boolean indicator of the operation's success, and sets the `http_loader`'s
// `connected` member appropriately.
bool http_loader::Connect(const std::string& host) {
	this->connected = false;
	close(this->sock);
	this->sock = socket(AF_INET, SOCK_STREAM, TCP->p_proto);
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
	URL parsedURL=url;

	if (url != this->url) {
		// need to check if this is the same host
		URL myURL = this->url;

		if (parsedURL.host != myURL.host) {
			this->connected = false;
		}
	}

	this->url = url;


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


	HTTPResponse* resp = nullptr;
	size_t totalReceived = 0;
	unsigned int tries = 0;

	do {
		std::memset(this->buffer, '\0', BUFFER_SIZE);
		dataSize = recv(this->sock, this->buffer, BUFFER_SIZE, 0);

		if (dataSize <= 0) {
			#ifdef DEBUG
			std::cout << "Disconnect detected; attempting reconnect" << std::endl;
			#endif

			if (!this->Connect(parsedURL.host)) {
				return stream;
			}
			else {
				tries++;

				if (tries > MAX_TRIES) {
					return stream;
				}
				continue;
			}

		}
		break;
	} while (true);

	totalReceived = dataSize;
	resp = new HTTPResponse(this->buffer, dataSize);

	// resp->add_body(this->buffer+resp->headerlen, dataSize-resp->headerlen);

	// stream->add_data(resp->, dataSize-headerlen);

	#ifdef DEBUG
	std::cout << "Recieved " << dataSize << " bytes from " << this->url << '.' << std::endl;
	#endif

	if (resp->content_length > 0) {
		stream->add_data(resp->body, dataSize-resp->headerlen);
		size_t remaining = resp->content_length+resp->headerlen - dataSize;

		#ifdef DEBUG
		std::cout << "Need " << remaining << " more bytes from " << parsedURL.host << " to fill out Content-Length: " << resp->content_length << std::endl;
		std::cout << "(remaining < resp.content_length)=" << (remaining < resp->content_length ? "true" : "false") << std::endl;
		#endif

		while (remaining > 0) {
			std::memset(this->buffer, '\0', BUFFER_SIZE);
			dataSize = recv(this->sock, this->buffer, BUFFER_SIZE, 0);

			#ifdef DEBUG
			std::cout << "Recieved " << dataSize << " bytes from " << this->url << '.' << std::endl;
			#endif

			stream->add_data(this->buffer, dataSize);
		}
	}

	else if (resp->transfer_encoding == "chunked" or resp->transfer_encoding == "Chunked") {
		#ifdef DEBUG
		std::cout << "Chunked encoding, fetching and parsing chunks" << std::endl;
		#endif

		std::string response = lstrip(std::string((char*)(resp->body)));
		std::string body = response;

		std::string chunk;
		// I don't know why, but if I remove this and try to re-use `chunklen` it won't work.
		unsigned int chunklen = 0;

		do {

			// This parses whatever's still in the 'response' buffer, and sets 'chunk' to
			// the contents of the most recent chunk, to the amount available. It also sets
			// 'CHUNKLEN' to the expected length of the chunk.
			while(!response.empty()) {

				unsigned int CHUNKLEN = 0;
				unsigned int pos = response.find("\r\n");

				// Our response is empty and/or doesn't contain delimiters
				if (pos == std::string::npos) {
					break;
				}

				#ifdef DEBUG
				std::cout << "=============== RESPONSE BUFFER ===============" << std::endl << response << std::endl << "===============================================" << std::endl;
				#endif

				std::string cl = response.substr(0, pos);

				#ifdef DEBUG
				std::cout << "Raw chunk length: '" << cl << '\'' << std::endl;
				#endif

				std::stringstream ss;
				ss << std::hex << cl;
				ss >> CHUNKLEN;

				chunklen = CHUNKLEN;

				#ifdef DEBUG
				std::cout << "Chunk length: " << chunklen << 'B' << std::endl;
				#endif

				response = response.substr(pos+2);

				if (response.length() >= chunklen) {
					chunk = response.substr(0, chunklen);
					response = lstrip(response.substr(chunklen));
				}
				else {
					chunk = response;
					response.clear();
				}

				body += chunk;

			}

			// If we reach here and the current chunk length is zero, then we must be done
			if (chunklen == 0) {
				break;
			}

			// ... Otherwise we have other chunks to fetch.
			// TODO - this causes an indefinite wait when the remaining data to be fetched is
			// an exact multiple of 'BUFFER_SIZE'
			do {
				std::memset(this->buffer, '\0', BUFFER_SIZE);
				dataSize = recv(this->sock, this->buffer, BUFFER_SIZE, 0);

				#ifdef DEBUG
				std::cout << "Recieved " << dataSize << " bytes from " << this->url << std::endl;
				#endif

				response += this->buffer;
			} while (dataSize == BUFFER_SIZE);

			if (chunk.length() < chunklen) {
				chunk += response.substr(0, chunklen);
				response = lstrip(response.substr(chunklen));
			}

		} while (chunklen > 0);
	}

	if (resp->content_type.first == "text") {
		std::cout << resp->toString() << std::endl;
		stream->add_data(resp->Body());
	}

	delete resp;
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
