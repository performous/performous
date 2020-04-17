
#pragma once

// Note: One could expect to have boost regex removed, but for some obscure
// reason, std::regex code crash on Fedora 30 (see bug #476). Thus, until FC is
// fixed or deprecated, may be eeded to keep this for a little while

#ifdef USE_BOOST_REGEX
#include <boost/regex.hpp>
using regex = boost::regex;
namespace regex_constants = boost::regex_constants;
#else
#include <regex>
using regex = std::regex;
namespace regex_constants = std::regex_constants;
#endif
