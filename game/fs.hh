#pragma once

#include <boost/filesystem/path.hpp>
#include <fstream>
#include <list>

namespace fs = boost::filesystem;

typedef std::vector<std::uint8_t> BinaryBuffer;

/// Read an entire file into a buffer
BinaryBuffer readFile(fs::path const& path);

/** Get user's home folder **/
fs::path getHomeDir();

/** Get the users configuration folder **/
fs::path getConfigDir();

/** Get the users data folder **/
fs::path getDataDir();

/** Get the users cache folder **/
fs::path getCacheDir();

/** Get the locale folder **/
fs::path getLocaleDir();

/** Get full path to a file from the current theme **/
fs::path getThemePath(fs::path const& filename);

/** Get available theme names **/
std::vector<std::string> getThemes();

/** Get full path to a share file **/
fs::path getPath(fs::path const& filename);

/** Get full path to a default conguration file **/
fs::path getDefaultConfig(fs::path const& configFile);

typedef std::list<fs::path> Paths;

/** Get all shared data paths in preference order **/
Paths const& getPaths(bool refresh = false);

/** Get all paths listed in a config option **/
Paths getPathsConfig(std::string const& confOption);

void resetPaths();

