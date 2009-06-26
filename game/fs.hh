#pragma once

#include <boost/filesystem.hpp>

// Define this useful alias for the overlong namespace name (yes, for everyone who includes this header)
namespace fs = boost::filesystem;

/** Get user's home folder **/
fs::path getHomeDir();

/** Do mangling to convert user-entered path into path suitable for use with stdlib etc. **/
fs::path pathMangle(fs::path const& dir);

/** Get full path to a file from the current theme **/
std::string getThemePath(std::string const& filename);

