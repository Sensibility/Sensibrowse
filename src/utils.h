#ifndef UTILS
#define UTILS

#include <locale>

#include "globals.h"

// en_US IS THE ONLY LOCALE!
static const std::locale LOCALE("en_US.UTF8");
// USA! USA! USA! USA! USA!
#define isspace(s) std::isspace(s, LOCALE)
#define isnumber(s) (!s.empty() && s.find_first_not_of("0123456789") == std::string::npos)

std::string urljoin(const std::string&, const std::string&);
std::vector<std::string> split(const std::string);
std::vector<std::string> split(const std::string, const std::string&);
std::string lstrip(const std::string&);
std::string lstrip(const std::string, const std::string&);
std::string rstrip(const std::string&);
std::string rstrip(const std::string, const std::string&);
std::string strip(const std::string&);
std::string strip(const std::string, const std::string&);

#endif
