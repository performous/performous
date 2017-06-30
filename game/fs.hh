#pragma once

#include <boost/filesystem/path.hpp>
#include <list>

namespace fs = boost::filesystem;

std::list<std::string> getThemes();  ///< Find all theme folders and return theme names.

/// Recursively copies a folder, throws on error.
void copyDirectoryRecursively(const fs::path& sourceDir, const fs::path& destinationDir);

/// Determine where the important system paths and most importantly the config schema are. Must be run before any of the functions below.
void pathBootstrap();

/// Do full data dir (search path) init or re-init after config options have been modified.
/// - Full search path and themes will only be available after this.
/// - Logging and config must be running before pathInit (pathInit is first called from configuration.cc).
void pathInit();

/// Test if a path begins with name and if so, remove that element and return true
/// Mostly a workaround for fs::path's crippled API that makes this operation difficult
bool pathRootHack(fs::path& p, std::string const& name);

fs::path execname(); ///< Get the path and filename of the main executable.
fs::path getLogFilename();  ///< Get the log filename.
fs::path getSchemaFilename();  ///< Get the config schema filename.
fs::path getHomeDir();  ///< Get user's home folder.
fs::path getConfigDir();  ///< Get user-writable Performous config folder.
fs::path getSysConfigDir();  ///< Get root-writable Performous config folder.
fs::path getDataDir();  ///< Get user-writable Performous data folder.
fs::path getCacheDir();  ///< Get user-writable temporary folder.
fs::path getShareDir();  ///< Get Performous system-level data folder.
fs::path getLocaleDir();  ///< Get the system local folder.

typedef std::list<fs::path> Paths;

struct pathCache {
	Paths pathExpand(fs::path p);
	void pathBootstrap();
	void pathInit();
};

fs::path findFile(fs::path const& filename);  ///< Look for the specified file in theme and data folders.
Paths listFiles(fs::path const& dir);  ///< List contents of specified folder in theme and data folders (omit duplicates).

Paths const& getPaths();  ///< Get the data file search path
Paths getThemePaths();  ///< Get the data/theme file search path (includes current and default themes in addition to data folders)
Paths getPathsConfig(std::string const& confOption);  ///< Return expanded list of paths specified by a path config option

