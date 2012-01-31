#pragma once

#include <boost/filesystem.hpp>
#include <vector>

// Define this useful alias for the overlong namespace name (yes, for everyone who includes this header)
namespace fs = boost::filesystem;

/** Get user's home folder **/
fs::path getHomeDir();

/** Get the users configuration folder **/
fs::path getConfigDir();

/** Get the users data folder **/
fs::path getDataDir();

/** Get the users cache folder **/
fs::path getCacheDir();

/** Get the users theme folder **/
fs::path getThemeDir();

/** Is the file a theme resource **/
bool isThemeResource(fs::path);

/** Get the localec folder **/
fs::path getLocaleDir();

/** Do mangling to convert user-entered path into path suitable for use with stdlib etc. **/
fs::path pathMangle(fs::path const& dir);

/** Get full path to a file from the current theme **/
std::string getThemePath(std::string const& filename);

/** Get available theme names **/
std::vector<std::string> getThemes();

/** Get full path to a share file **/
std::string getPath(fs::path const& filename);

/** Get full path to a default conguration file **/
fs::path getDefaultConfig(fs::path const &configFile);

typedef std::vector<fs::path> Paths;

/** Get all shared data paths in preference order **/
Paths const& getPaths(bool refresh = false);

/** Get all paths listed in a config option **/
Paths getPathsConfig(std::string const& confOption);
