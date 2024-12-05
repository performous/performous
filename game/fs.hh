#pragma once

#include "config.hh"

#include <cstdint>
#include <filesystem>
#include <list>
#include <vector>

namespace fs {
	using namespace std::filesystem;
	using std::ifstream;
	using std::fstream;
	using std::ofstream;
}

using Paths = std::list<fs::path>;

namespace {
	struct PathCache {
		// Note: three-phase init:
		// 1. Default constructor runs in static context (before main) and cannot do much		
		// 2. pathBootstrap is called to find out static system paths (critical for logging and for loading config files)
		// 3. pathInit is called to process the full search path, using config settings

		Paths paths;
		bool didMigrateConfig = false;
		fs::path base, share, locale, sysConf, home, conf, data, cache;

		/// Expand a path specifier as a list of actual paths. Expands ~ (home) and DATADIR (Performous search path).
		Paths pathExpand(fs::path p);

		void pathBootstrap();
		void pathInit();
	} cache;
}

// Reimplment boost's absolute function with 2 parameters, according to its documentation:
// https://www.boost.org/doc/libs/1_51_0/libs/filesystem/doc/reference.html#absolute
static inline fs::path absolute(const fs::path& p, const fs::path& base) {
	if (p.has_root_directory()) {
		if (p.has_root_name())
			return p;
		else
			return fs::absolute(base).root_name() / p;
	} else {
		if (p.has_root_name())
			return p.root_name() / fs::absolute(base).root_directory() / fs::absolute(base).relative_path() / p.relative_path();
		else
			return fs::absolute(base) / p;
	}
}

using BinaryBuffer = std::vector<std::uint8_t>;

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
fs::path getLogFilename_new();  ///< Get the log filename.
fs::path getSchemaFilename();  ///< Get the config schema filename.
fs::path getHomeDir();  ///< Get user's home folder.
fs::path getConfigDir();  ///< Get user-writable Performous config folder.
fs::path getSysConfigDir();  ///< Get root-writable Performous config folder.
fs::path getDataDir();  ///< Get user-writable Performous data folder.
fs::path getCacheDir();  ///< Get user-writable temporary folder.
fs::path getShareDir();  ///< Get Performous system-level data folder.
fs::path getLocaleDir();  ///< Get the system local folder.

fs::path findFile(fs::path const& filename);  ///< Look for the specified file in theme and data folders.

BinaryBuffer readFile(fs::path const& path); ///< Reads a file into a buffer. 

Paths listFiles(fs::path const& dir);  ///< List contents of specified folder in theme and data folders (omit duplicates).

Paths const& getPaths();  ///< Get the data file search path
Paths getThemePaths();  ///< Get the data/theme file search path (includes current and default themes in addition to data folders)
Paths getPathsConfig(std::string const& confOption);  ///< Return expanded list of paths specified by a path config option

struct FsPathHash {
	size_t operator()(const fs::path& path) const noexcept {
		return fs::hash_value(path);
	}
};