#include "url.h"

static const char* KNOWN_PROTOCOLS[] = {
	"HTTP",
	"HTTPS",
	"FTP",
	"FILE",
	"ABOUT",
	"SSH"
};

// this is pretty terrible tbh; I'm iterating the url potentially like 16 times
// and it won't allow you to put '#' in the value of a query paramater,
// but it'll be serviceable for a first pass.

URL::URL(const std::string& url) {
	std::string currentURL = url;

	size_t pos = url.find("://");
	if (pos != std::string::npos) {
		this->protocol = url.substr(0, pos);
		currentURL = url.substr(pos+3);
	}

	pos = currentURL.find('/');
	size_t auxPos = currentURL.find('@');
	if (auxPos != std::string::npos && auxPos < pos) {
		size_t anotherAuxPos = currentURL.find(':');
		if (anotherAuxPos != std::string::npos && anotherAuxPos < auxPos && anotherAuxPos < pos) {
			this->username = currentURL.substr(0, anotherAuxPos);
			currentURL = currentURL.substr(anotherAuxPos+1);
			auxPos = currentURL.find('@');
			this->password = currentURL.substr(0, auxPos);
			currentURL = currentURL.substr(auxPos+1);
		}
		else {
			this->username = currentURL.substr(0, auxPos);
			currentURL = currentURL.substr(auxPos+1);
		}
	}

	pos = currentURL.find('/');
	this->host = currentURL.substr(0, pos);

	if (pos != std::string::npos) {
		currentURL = currentURL.substr(pos);

		// now everything up to a '#' or a '?' is the request path
		pos = currentURL.find('#');
		auxPos = currentURL.find('?');
		char which = '\0';
		if (pos != std::string::npos && pos < auxPos) {
			this->path = currentURL.substr(0, pos);
			this->anchor = currentURL.substr(pos, auxPos - pos);
			currentURL = currentURL.substr(auxPos);
			which = '?';
		}
		else if (auxPos != std::string::npos && auxPos < pos) {
			this->path = currentURL.substr(0, auxPos);
			this->query_params = currentURL.substr(auxPos, pos-auxPos);
			currentURL = currentURL.substr(pos);
			which = '#';
		}
		else {
			this->path = currentURL;
			auxPos = std::string::npos;
		}

		switch (which) {
			case '?':
				this->query_params = currentURL;
				break;
			case '#':
				this->anchor = currentURL;
				break;
			default:
				break;
		}


	}
	else {
		// url ended immediately after host; path is implicitly '/'
		// no anchor and no query params
		this->path = '/';
	}

}

URL::URL(const char* rhs) {

	// URL* this;
	std::string currentURL = rhs;

	size_t pos = currentURL.find("://");
	if (pos != std::string::npos) {
		this->protocol = currentURL.substr(0, pos);
		currentURL = currentURL.substr(pos+3);
	}

	pos = currentURL.find('/');
	size_t auxPos = currentURL.find('@');
	if (auxPos != std::string::npos && auxPos < pos) {
		size_t anotherAuxPos = currentURL.find(':');
		if (anotherAuxPos != std::string::npos && anotherAuxPos < auxPos && anotherAuxPos < pos) {
			this->username = currentURL.substr(0, anotherAuxPos);
			currentURL = currentURL.substr(anotherAuxPos+1);
			auxPos = currentURL.find('@');
			this->password = currentURL.substr(0, auxPos);
			currentURL = currentURL.substr(auxPos+1);
		}
		else {
			this->username = currentURL.substr(0, auxPos);
			currentURL = currentURL.substr(auxPos+1);
		}
	}

	pos = currentURL.find('/');
	this->host = currentURL.substr(0, pos);

	if (pos != std::string::npos) {
		currentURL = currentURL.substr(pos);

		// now everything up to a '#' or a '?' is the request path
		pos = currentURL.find('#');
		auxPos = currentURL.find('?');
		char which = '\0';
		if (pos != std::string::npos && pos < auxPos) {
			this->path = currentURL.substr(0, pos);
			this->anchor = currentURL.substr(pos, auxPos - pos);
			currentURL = currentURL.substr(auxPos);
			which = '?';
		}
		else if (auxPos != std::string::npos && auxPos < pos) {
			this->path = currentURL.substr(0, auxPos);
			this->query_params = currentURL.substr(auxPos, pos-auxPos);
			currentURL = currentURL.substr(pos);
			which = '#';
		}
		else {
			this->path = currentURL;
			auxPos = std::string::npos;
		}

		switch (which) {
			case '?':
				this->query_params = currentURL;
				break;
			case '#':
				this->anchor = currentURL;
				break;
			default:
				break;
		}


	}
	else {
		// url ended immediately after host; path is implicitly '/'
		// no anchor and no query params
		this->path = '/';
	}

}

URL::~URL() {

}

std::string URL::toString() {
	std::string ret = this->protocol;

	if (!ret.empty()) {
		ret+="://";
	}

	if (!(this->username).empty()) {
		ret += this->username;

		if (!(this->password).empty()) {
			ret += ':' + this->password;
		}

		ret += '@';
	}

	ret += this->host + this->path;

	if (!(this->anchor).empty()) {
		ret += '#' + this->anchor;
	}

	if (!(this->query_params).empty()) {
		ret += '?' + this->query_params;
	}

	return ret;
}

std::string URL::repr() {
	std::string ret = "URL(protocol=\"";
	ret += this->protocol;
	ret += "\", username=\"";
	ret += this->username;
	ret += "\", password=\"";
	ret += this->password;
	ret += "\", host=\"";
	ret += this->host;
	ret += "\", path=\"";
	ret += this->path;
	ret += "\", anchor=\"";
	ret += this->anchor;
	ret += "\", query_params=\"";
	ret += this->query_params;
	ret += "\")";

	return ret;
}
