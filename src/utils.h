#ifndef UTILS
#define UTILS

#include <locale>

#include "globals.h"

static const std::locale LOCALE("en_US.UTF8");

#define isspace(s) std::isspace(s, LOCALE)

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
