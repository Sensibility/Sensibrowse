#include "http.h"

// Constructs an HTTP response from the `raw_response` buffer of length `amt`
HTTPResponse::HTTPResponse(char* raw_response, const size_t& amt) {
	// The first line MUST be the status line of the request.
	// So, the first thing to do is find a line ending (crlf)
	size_t pos;
	this->headerlen=0;
	bool cr = false;

	for (pos = 0; pos < amt; ++pos) {
		if (cr) {
			if (*(raw_response + pos) == '\n') {
				--pos;
				break;
			}
			cr = false;
		}
		else if (*(raw_response + pos) == '\r') {
			cr = true;
		}
	}

	// Now that we have the first line, we can set the status line
	char sl[pos+1] = {'\0'};
	this->setStatusline(std::string((char*)(std::memcpy(sl, raw_response, pos))));

	this->headerlen = pos+2;

	// Now each line that follows until a double crlf must be a colon-separated
	// key/value pair representing an HTTP header.

	char* totalOffset = raw_response + pos + 2; // Indicates the read position
	cr = false;
	bool crlf = false;

	while (totalOffset < raw_response+amt &&
	       totalOffset - raw_response + 1 < amt &&
	       !(*(totalOffset) == '\r' && *(totalOffset + 1) == '\n')) {

		size_t colonPos = 0;
		for (pos = 0; pos < (totalOffset - raw_response) + amt; ++pos) {
			if (cr) {
				if (*(totalOffset+pos) == '\n') {
					--pos;
					break;
				}
				cr = false;
			}
			else if (*(totalOffset+pos) == '\r') {
				cr = true;
			}
			// This marks the location of the first-encountered colon.
			// TODO - this will treat the sequence '\\:' as an escaped colon.
			else if (!colonPos && pos && *(totalOffset+pos) == ':' && *(totalOffset+pos-1) != '\\') {
				colonPos = pos;
			}
		}

		// Now, `pos` should be the length of the string starting with `totalOffset` and ending
		// right before the crlf sequence signaling a line ending. If a colon delimiter was found,
		// it will be indicated in `colonPos`.

		if (colonPos) {
			char header[colonPos+1] = {'\0'};
			char value[pos-colonPos] = {'\0'};
			std::pair<std::string, std::string> newHeader = {
			                                                    rstrip(std::string((char*)(std::memcpy(header, totalOffset, colonPos)))),
			                                                    strip(std::string((char*)(std::memcpy(value, totalOffset+colonPos+1, pos-colonPos-1))))
			                                                };
			this->headers.push_back(newHeader);

			// Now we do checks for special headers
			if (newHeader.first == "Content-Length") {
				this->content_length = std::atoi(newHeader.second.c_str());
			}
			else if (newHeader.first == "Content-Type") {
				size_t slashPos = newHeader.second.find('/');
				if (slashPos && slashPos != std::string::npos){
					this->content_type = {newHeader.second.substr(0, slashPos), newHeader.second.substr(slashPos+1)};
				}
				else {
					this->content_type = {newHeader.second, std::string("")};
				}
			}
			else if (newHeader.first == "Transfer-Encoding") {
				this->transfer_encoding = newHeader.second;
			}
		}
		#ifdef DEBUG
		else {
			char tmp[pos+1] = {'\0'};
			std::cerr << "Bad header: '" << std::memcpy(tmp, totalOffset, pos) << '\'' << std::endl;
		}
		#endif

		// Move read position to beginning of next line (loop condition checks for cr/lf)
		totalOffset += 2+pos;
		this->headerlen += 2+pos;
	}
	this->headerlen += 2;
}

HTTPResponse::HTTPResponse(const std::string& raw_response) {
	std::stringstream responseStream(raw_response);
	std::string line;
	bool done_with_headers = false;
	size_t pos;

	std::getline(responseStream, line, '\n');
	this->setStatusline(line);

	std::string BODY;
	while (std::getline(responseStream, line, '\n')) {
		if (done_with_headers) {
			BODY += line + '\n';
		}
		else if (line.empty() || line == "\r") {
			done_with_headers = true;
		}
		else {
			pos = line.find(':');
			std::pair<std::string, std::string> newHeader = {line.substr(0, pos), line.substr(pos+1)};
			this->headers.push_back(newHeader);
			if (newHeader.first == "Transfer-Encoding" || newHeader.first == "transfer-encoding") {
				this->transfer_encoding = strip(newHeader.second);
			}
			else if (newHeader.first == "Content-Length" || newHeader.first == "content-length") {
				this->content_length = std::atoi(strip(newHeader.second).c_str());
			}
			else if (newHeader.first == "Content-Type" || newHeader.first == "content-type") {
				pos = newHeader.second.find('/');
				if (pos != std::string::npos) {
					this->content_type = {strip(newHeader.second.substr(0, pos)), strip(newHeader.second.substr(pos+1))};
				}
			}
		}
	}

	this->body = (void*)(BODY.c_str());
}


void HTTPResponse::setStatusline(std::string sl) {
	this->statusline = sl;

	size_t pos = sl.find(' ');
	if ( pos == std::string::npos) {
		#ifdef DEBUG
		std::cout << "Bad status line on request: '" << sl << '\'' << std::endl;
		#endif
		this->protocol = "HTTP/?.?";
		this->status = 0;
		this->reason = "UNKNOWN";
		return;
	}
	this->protocol = sl.substr(0, pos);
	sl = sl.substr(pos+1);

	pos = sl.find(' ');
	if ( pos == std::string::npos) {
		#ifdef DEBUG
		std::cout << "Bad status line on request: '" << sl << '\'' << std::endl;
		#endif
		this->status = 0;
		this->reason = "UNKNOWN";
		return;
	}
	std::string status = strip(sl.substr(0, pos));
	if ( isnumber(status) ) {
		this->status = std::stoul(status);
	}
	else {
		this->status = 0;
		#ifdef DEBUG
		std::cout << "Error parsing status, '" << status << "' is not a number!" << std::endl;
		#endif
	}
	this->status = std::stoul(sl.substr(0, pos));
	this->reason = sl.substr(pos+1);
}

// This function will return the body of the response, if it is representable
// as text - otherwise an empty string.
std::string HTTPResponse::Body() {
	// If chunked transfer encoding is used, the content_length will be set
	// after all content has been read.
	if (!this->content_length || this->content_type.first != "text") {
		return std::string("");
	}

	// TODO - support UTF encodings.
	char buff[this->content_length+1] = {'\0'};
	return std::string((char*)(std::memcpy(buff, this->body, this->content_length)));
}

std::string HTTPResponse::toString() {
	std::string ret = this->statusline + "\r\n";
	for (std::pair<std::string,std::string> h : this->headers) {
		ret += h.first + ": " + h.second + "\r\n";
	}
	return ret + "\r\n" + this->Body();
}

HTTPRequest::HTTPRequest(const char* raw_request) {
	std::stringstream requestStream(raw_request);
	std::string line;
	bool done_with_headers = false;
	size_t pos;

	std::getline(requestStream, line, '\n');
	this->setRequestline(line);

	while (std::getline(requestStream, line, '\n')) {
		if (done_with_headers) {
			this->body += line + '\n';
		}
		else if (line.empty() || line == "\r") {
			done_with_headers = true;
		}
		else {
			pos = line.find(':');
			std::pair<std::string, std::string> newHeader = {line.substr(0, pos), line.substr(pos+1, line.length()-pos-2)};
			while (newHeader.second[0] == ' ') {
				newHeader.second = newHeader.second.substr(1);
			}
			this->headers.push_back(newHeader);
		}
	}
}

std::string HTTPRequest::toString() {
	std::string ret = this->requestline + "\r\n";
	for (auto h : this->headers) {
		ret += h.first + ':' + h.second + "\r\n";
	}
	return ret + "\r\n" + this->body;
}


void HTTPRequest::setRequestline(std::string rl) {
	this->requestline = rl;

	size_t pos = rl.find(' ');
	this->method = rl.substr(0, pos);
	rl = rl.substr(pos+1);

	pos = rl.find(' ');
	this->path = rl.substr(0, pos);
	this->protocol = rl.substr(pos+1);
}

HTTPResponse HTTPRequest::Send(const int& sock) {
	// Note that this method takes in a socked file descriptor,
	// and expects it to be in the 'connected' state
	std::string req = this->toString();
	size_t datasize = send(sock, req.c_str(), req.length(), 0);

	#ifdef DEBUG
	std::cout << "Sent " << datasize << " bytes" << std::endl;
	#endif

	while (datasize < req.length()) {
		std::string sub = req.substr(datasize);
		size_t subSize = send(sock, sub.c_str(), sub.length(), 0);

		#ifdef DEBUG
		std::cout << "Sent " << subSize << " bytes" << std::endl;
		#endif

		datasize += subSize;
	}


	// Now receive the response
	char buff[BUFFER_SIZE] = {'\0'};

	datasize = recv(sock, buff, BUFFER_SIZE, 0);

	#ifdef DEBUG
	std::cout << "Received " << datasize << " bytes" << std::endl;
	#endif

	HTTPResponse resp = HTTPResponse(buff, datasize);

	#ifdef DEBUG
	std::cout << "Response:" << std::endl << resp.toString() << std::endl;
	#endif

	if (resp.transfer_encoding == "chunked") {
		#ifdef DEBUG
		std::cout << "Transfer-Encoding was chunked, need to retrieve chunks" << std::endl;
		#endif

		datasize = recv(sock, buff, BUFFER_SIZE, 0);

		#ifdef DEBUG
		std::cout << "Received " << datasize << " bytes" << std::endl;
		std::cout << buff << std::endl;
		#endif
	}

	return resp;

}
