#include "utils.h"

std::string urljoin(const std::string &base, const std::string &relative)
{
	try
	{
		Poco::URI uri_base(base);
		Poco::URI uri_res(uri_base, relative);
		return uri_res.toString();
	} catch (...)
	{
		return relative;
	}
}


// Splits the string `splitMe` on contiguous whitespace segments, and returns the
// resulting list of strings as a std::vector of strings.
std::vector<std::string> split(const std::string splitMe) {
	std::vector<std::string> ret;
	std::string tmp = "";

	for (unsigned int i = 0; i < splitMe.length(); ++i) {
		if (isspace(splitMe[i])) {
			ret.push_back(tmp);
			++i;
			while (i < splitMe.length() && isspace(splitMe[i])) {
				++i;
			}

			if ( i < splitMe.length()) {
				tmp = splitMe[i];
			}
			else {
				tmp = "";
			}
		}
		else {
			tmp.push_back(splitMe[i]);
		}
	}

	ret.push_back(tmp);
	return ret;
}


// Splits the string `splitMe` wherever the string `splitOn` occurs, and returns the
// resulting list of strings as a std::vector of strings.
std::vector<std::string> split(const std::string splitMe, const std::string& splitOn) {
	std::vector<std::string> ret;
	unsigned int splitterLen = splitOn.length();

	// In either of these cases, it's impossible for `splitMe` to be split,
	// so we return a 1-entry vector containing the original string.
	if (splitterLen <= 0 || splitterLen > splitMe.length()) {
		ret.push_back(splitMe);
		return ret;
	}

	std::string tmp = "";
	for (unsigned int i = 0; i <= splitMe.length() - splitterLen; ++i ) {
		if (splitMe.substr(i, splitterLen) == splitOn) {
			ret.push_back(tmp);
			tmp = "";
			i += splitterLen;
		}
		else {
			tmp.push_back(splitMe[i]);
		}
	}

	ret.push_back(tmp);

	return ret;
}


// Strips whitespace from the left side (beginning) of the string `stripMe` and returns
// the result
std::string lstrip(const std::string& stripMe) {
	unsigned int i;
	for (i = 0; i < stripMe.length() && isspace(stripMe[i]); ++i) {
		//nothing to do here.
	}

	return stripMe.substr(i);
}

// Strips the string specified by `stripThis` from the left side (beginning) of the string
// `stripMe` and returns the result
std::string lstrip(const std::string stripMe, const std::string& stripThis) {
	unsigned int striplen = stripThis.length();

	// in either case, this means there's nothing to strip.
	if (striplen == 0 || striplen > stripMe.length()) {
		return stripMe;
	}

	unsigned int i;
	for (i = 0; i <= stripMe.length() - striplen; i+=striplen) {
		if (stripMe.substr(i, striplen) != stripThis) {
			break;
		}
	}

	return stripMe.substr(i);
}

// Strips whitespace from the right side (end) of the string `stripMe` and returns the result
std::string rstrip(const std::string& stripMe) {
	unsigned int i = stripMe.length() - 1;
	for (i; i >= 0 && isspace(stripMe[i]); --i){
		//nothing to do here
	}

	return stripMe.substr(0, i+1);
}

// Strips the string specified by `stripThis` from the right side (end) of the string `stripMe`
// and returns the result
std::string rstrip(const std::string stripMe, const std::string& stripThis) {
	unsigned int striplen = stripThis.length();

	// in either case, this means there's nothing to strip.
	if (striplen == 0 || striplen > stripMe.length()) {
		return stripMe;
	}

	unsigned int i;
	for (i = stripMe.length() - striplen - 1; i >= 0; i-=striplen) {
		if (stripThis.substr(i, striplen) != stripThis) {
			break;
		}
	}

	return stripMe.substr(0, i+1);
}

// Convenience function to strip all leading and trailing whitespace from a string
std::string strip(const std::string& stripMe) {
	return rstrip(lstrip(stripMe));
}

// Convenience function to strip all leading and trailing instances of `stripThis` from
// a string specified by `stripMe`
std::string strip(const std::string stripMe, const std::string& stripThis) {
	return rstrip(lstrip(stripMe, stripThis), stripThis);
}
