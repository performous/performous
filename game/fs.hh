#pragma once

#include <list>
#include <vector>

#ifdef USE_BOOST_FS
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

	#if BOOST_VERSION < 107400
	// create_directory was added in boost 1.74
	// see https://www.boost.org/users/history/version_1_74_0.html

	// in boost, copy_directory means create_directory
	// https://www.boost.org/doc/libs/1_66_0/libs/filesystem/doc/reference.html#copy_directory
	static inline void create_directory(const fs::path &to, const fs::path &from) {
	    copy_directory(from, to);
	}

	#endif

#else
#include <filesystem>
namespace fs {

using namespace std::filesystem;

using std::ifstream;
using std::fstream;
using std::ofstream;

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

#endif

typedef std::vector<std::uint8_t> BinaryBuffer;

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

BinaryBuffer readFile(fs::path const& path); ///< Reads a file into a buffer. 

Paths listFiles(fs::path const& dir);  ///< List contents of specified folder in theme and data folders (omit duplicates).

Paths const& getPaths();  ///< Get the data file search path
Paths getThemePaths();  ///< Get the data/theme file search path (includes current and default themes in addition to data folders)
Paths getPathsConfig(std::string const& confOption);  ///< Return expanded list of paths specified by a path config option

template <>
class std::hash<fs::path>
{
public:
    size_t operator()(const fs::path& path) const
    {
        return fs::hash_value(path);
    }
};