#include "http_loader.h"
#include "url.h"
#include "http.h"

http_loader::http_loader()
{
	sock = socket(AF_INET, SOCK_STREAM, TCP->p_proto);
}

http_loader::~http_loader()
{
	close(sock);
}

size_t http_loader::curlWriteFunction( void *ptr, size_t size, size_t nmemb, void *stream )
{
	Glib::RefPtr< Gio::MemoryInputStream >* s = (Glib::RefPtr< Gio::MemoryInputStream >*) stream;
	(*s)->add_data(ptr, size * nmemb);
	return size * nmemb;
}

Glib::RefPtr< Gio::InputStream > http_loader::load_url(const litehtml::tstring& url)
{
	this->url = url;

	URL parsedURL=url;

	#if DEBUG
	std::cout << "Fetching via GET: " << url << " - " << parsedURL.repr() << std::endl;
	#endif

	Glib::RefPtr< Gio::MemoryInputStream > stream = Gio::MemoryInputStream::create();

	struct addrinfo* addrs;
	struct addrinfo* addr;
	if (getaddrinfo((parsedURL.host).c_str(), "80", &hint, &addrs) || addrs == nullptr) {
		std::cerr << "Error fetching url '" << parsedURL.toString() << '\'' << ": Host not known" << std::endl;
		return stream;
	}

	for (addr = addrs; addr != nullptr; addr = addr->ai_next) {
		std::cout << "Fetching..." << std::endl;
		if (addr->ai_addrlen > 0 && addr->ai_family == AF_INET) {
			break;
		}
	}

	if (addr == nullptr) {
		std::cerr << "Error making connection" << std::endl;
		return stream;
	}

	#ifdef DEBUG
	std::cout << "Got the addrinfo" << std::endl << "Conecting..." << std::endl << std::endl;
	#endif

	if (connect(this->sock, addrs->ai_addr, addrs->ai_addrlen)) {
		std::cerr << "Error making connection to: " << parsedURL.toString() << std::endl;
		return stream;
	}

	#ifdef DEBUG
	std::cout << "Connected." << std::endl;
	#endif

	std::string request = "GET " + parsedURL.path + " HTTP/1.1\r\nConnection: Keep-Alive\r\nAccept-Encoding: identity\r\nAccept: text/html,text/plain;q=0.9,text/*;q=0.8,*/*;q=0.7\r\nAccept-Charset: utf-8\r\nAccept-Language: en-US\r\nDNT: 1\r\nUser-Agent: sensibrowse/0.1\r\nScript: paradisi,wasm,none;q=0.9,javascript;q=0.8\r\nHost: " + parsedURL.host + "\r\n\r\n";
	std::cout << request;

	size_t dataSize = send(this->sock, request.c_str(), request.length(), 0);

	#ifdef DEBUG
	std::cout << "Sent " << dataSize << " bytes of data to " << this->url << '.' << std::endl;
	#endif

	std::string response = "";

	do {
		char buff[4096] = {'\0'};
		dataSize = recv(this->sock, buff, 4096, 0);

		response += buff;

		#ifdef DEBUG
		std::cout << "Recieved " << dataSize << " bytes from " << this->url << '.' << std::endl;
		#endif

	} while (dataSize == 4096);

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
				char buff[4096] = {'\0'};
				dataSize = recv(this->sock, buff, 4096, 0);

				#ifdef DEBUG
				std::cout << "Recieved " << dataSize << " bytes from " << this->url << std::endl;
				#endif

				response += buff;
			} while (dataSize == 4096);

			pos = response.find("\r\n");

			if (pos == std::string::npos) {
				break;
			}

			std::string tmp = response.substr(0, pos);

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

	stream->add_data(resp.toString());

	// if(m_curl)
	// {
	//     curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
	//     curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &stream);
	//     curl_easy_perform(m_curl);
	//     char* new_url = NULL;
	//     if(curl_easy_getinfo(m_curl, CURLINFO_EFFECTIVE_URL, &new_url) == CURLE_OK)
	//     {
	//         if(new_url)
	//         {
	//             m_url = new_url;
	//         }
	//     }
	// }

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
