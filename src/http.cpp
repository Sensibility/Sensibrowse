#include "http.h"

HTTPResponse::HTTPResponse(const char* raw_response) {
	std::stringstream responseStream(raw_response);
	std::string line;
	bool done_with_headers = false;
	size_t pos;

	std::getline(responseStream, line, '\n');
	this->setStatusline(line);

	while (std::getline(responseStream, line, '\n')) {
		if (done_with_headers) {
			this->body += line + '\n';
		}
		else if (line.empty() || line == "\r") {
			done_with_headers = true;
		}
		else {
			pos = line.find(':');
			std::pair<std::string, std::string> newHeader = {line.substr(0, pos), line.substr(pos+1)};
			this->headers.push_back(newHeader);
			if (newHeader.first == "Transfer-Encoding" || newHeader.second == "transfer-encoding") {
				this->transfer_encoding = strip(newHeader.second);
			}
		}
	}
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

std::string HTTPResponse::toString() {
	std::string ret = this->statusline + "\r\n";
	for (auto h : this->headers) {
		ret += h.first + ": " + h.second + "\r\n";
	}
	return ret + "\r\n" + this->body;
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

	HTTPResponse resp = buff;

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
