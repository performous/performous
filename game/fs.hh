#pragma once

#include "config.hh"

#include <fmt/format.h>

#include <cstdint>
#include <filesystem>
#include <list>
#include <string_view>
#include <vector>
#include <mutex>

namespace fs {
	using namespace std::filesystem;
	using std::ifstream;
	using std::fstream;
	using std::ofstream;
}

using Paths = std::list<fs::path>;
using Lock  = std::scoped_lock<std::recursive_mutex>;
using BinaryBuffer = std::vector<std::uint8_t>;

fs::path execname(); ///< Get the path and filename of the main executable. (see execname.cc)
BinaryBuffer readFile(fs::path const& path); ///< Reads a file into a buffer.


class PathCache {

public:
	// PathCache is a singleton class that manages the system paths used by Performous.
	PathCache& operator=(const PathCache&) = delete;  // Disable assignment operator

	// PathCache is initialized in three phases:
	static PathCache &getInstance() {
		static PathCache instance;  // Guaranteed to be destroyed, instantiated on first use
		return instance;
	}

	const std::string CONFIG_SCHEMA{ "config/schema.xml" }; ///< The config schema file name."}
	const std::string INFO_LOG{ "infolog.txt" }; ///< The infolog file name.
	const fs::path    PERFORMOUS_PATH{"performous"};

	/// Determine where the important system paths and most importantly the config schema are. Must be run before any of the functions below.
	void pathBootstrap();

	/// Do full data dir (search path) init or re-init after config options have been modified.
	/// - Full search path and themes will only be available after this.
	/// - Logging and config must be running before pathInit (pathInit is first called from configuration.cc).
	void pathInit();

	/// Expand a path specifier as a list of actual paths. Expands ~ (home) and DATADIR (Performous search path).
	Paths pathExpand(fs::path p);

	std::string formatPath(const fs::path& target);
	fs::path    getLogFilename() { Lock l(m_mutex); return m_cache / INFO_LOG; }  ///< Get the log filename.
	fs::path    getProfilerLogFilename(); /// Profiler get its own log, or it obliterates everything else.
	fs::path    getSchemaFilename() { Lock l(m_mutex); return m_share / CONFIG_SCHEMA; } /// Get the config schema filename.
	fs::path    getBaseDir() { Lock l(m_mutex); return m_base; }  ///< Get Performous base folder.
	fs::path    getHomeDir() { Lock l(m_mutex); return m_home; } ///< Get user's home folder.
	fs::path    getShareDir() { Lock l(m_mutex); return m_share; } ///< Get Performous system-level data folder.
	fs::path    getLocaleDir() { Lock l(m_mutex); return m_locale; } ///< Get the system locale folder.
	fs::path    getConfigDir() { Lock l(m_mutex); return m_conf; }  ///< Get user-writable Performous config folder.
	fs::path    getSysConfigDir() { Lock l(m_mutex); return m_sysConf; }  ///< Get root-writable Performous config folder.
	fs::path    getDataDir() { Lock l(m_mutex); return m_data; } ///< Get user-writable Performous data folder.
	fs::path    getCacheDir() { Lock l(m_mutex); return m_cache; }  ///< Get user-writable temporary folder.


	Paths const& getPaths() { Lock l(m_mutex); return m_paths; } ///< Get the data file search path (includes current and default themes in addition to data folders)
	Paths getThemePaths();  ///< Get the data/theme file search path (includes current and default themes in addition to data folders)
	Paths getPathsConfig(std::string const& confOption);  ///< Return expanded list of paths specified by a path config option

	fs::path     findFile(fs::path const& filename); ///< Look for the specified file in theme and data folders.
	Paths        listFiles(fs::path const& dir);  ///< List contents of specified folder in theme and data folders (omit duplicates).

	std::list<std::string> getThemes(); ///< Get a list of available themes by searching for "themes" folders in the data paths.


    // Static Functions

	/// Recursively copies a folder, throws on error.
	static void copyDirectoryRecursively(const fs::path& sourceDir, const fs::path& destinationDir);

	/// Test if a path begins with name and if so, remove that element and return true
	/// Mostly a workaround for fs::path's crippled API that makes this operation difficult
	static bool pathRootHack(fs::path& p, std::string const& name);

    /// return the cannonical path of <target>
	static fs::path getCanonicalPath(const fs::path &target);


protected:
	std::recursive_mutex m_mutex;
	Paths m_paths{};
	bool didMigrateConfig{false};
	fs::path m_cache;
	fs::path m_base, m_share, m_locale, m_sysConf, m_home, m_conf, m_data;



private:
	// constructor is private to prevent instantiation
	PathCache()
	{
		// pathBootstrap is called to find out static system paths (critical for logging and for loading config files)
		// pathInit is called to process the full search path, using config settings
		//pathBootstrap();
		//pathInit();  init is part of bootstrap
	}

	// Delete all these because we're a singleton class.
	PathCache(const PathCache&) = delete;
	virtual ~PathCache() = default;
	PathCache(PathCache&&) = delete;
	PathCache& operator=(PathCache&&) = delete;

};  // class PathCache



/**
  * \brief  findFile is used all over the place, this permits less code modifications
  */
inline fs::path findFile(fs::path const& filename) ///< Look for the specified file in theme and data folders.
{
	return PathCache::getInstance().findFile(filename); // Call the singleton instance to find the file
}



// Make std::filesystem::path formattable.
template <>
struct fmt::formatter<std::filesystem::path>: formatter<std::string_view>
{
	template <typename FormatContext>
	auto format(const std::filesystem::path& path, FormatContext& ctx) const 
	{
		PathCache &pc = PathCache::getInstance();
		auto result = formatter<std::string_view>::format(pc.formatPath(path), ctx);  // NOTE: using the Cache
		return result;
	}
};

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

struct FsPathHash {
	size_t operator()(const fs::path& path) const noexcept {
		return fs::hash_value(path);
	}
};
