#pragma once

#include "config.hh"

#include <fmt/format.h>

#include <cstdint>
#include <filesystem>
#include <list>
#include <mutex>
#include <string_view>
#include <vector>

namespace fs {
	using namespace std::filesystem;
	using std::ifstream;
	using std::fstream;
	using std::ofstream;
}

using Paths = std::list<fs::path>;
using Lock = std::lock_guard<std::mutex>;

struct PathCache {
	// Note: three-phase init:
	// 1. Default constructor runs in static context (before main) and cannot do much
	// 2. pathBootstrap is called to find out static system paths (critical for logging and for loading config files)
	// 3. pathInit is called to process the full search path, using config settings
  private:
	static std::mutex m_mutex;

	static Paths paths;
	static inline bool didMigrateConfig = false;
	static fs::path base, share, locale, sysConf, home, conf, data, cache;

  public:
	/// Expand a path specifier as a list of actual paths. Expands ~ (home) and DATADIR (Performous search path).
	static Paths pathExpand(fs::path p);

	/// Determine where the important system paths and most importantly the config schema are. Must be run before any of the functions below.
	static void pathBootstrap();
	
	/// Do full data dir (search path) init or re-init after config options have been modified.
	/// - Full search path and themes will only be available after this.
	/// - Logging and config must be running before pathInit (pathInit is first called from configuration.cc).
	static void pathInit();
	
	static const fs::path getLogFilename();  ///< Get the log filename.
	static const fs::path getProfilerLogFilename();  ///< Profiler get its own log, or it obliterates everything else.
	static const fs::path getSchemaFilename();  ///< Get the config schema filename.
	static fs::path const& getBaseDir();  ///< Get Performous base folder.
	static fs::path const& getHomeDir();  ///< Get user's home folder.
	static fs::path const& getConfigDir();  ///< Get user-writable Performous config folder.
	static fs::path const& getSysConfigDir();  ///< Get root-writable Performous config folder.
	static fs::path const& getDataDir();  ///< Get user-writable Performous data folder.
	static fs::path const& getCacheDir();  ///< Get user-writable temporary folder.
	static fs::path const& getShareDir();  ///< Get Performous system-level data folder.
	static fs::path const& getLocaleDir();  ///< Get the system local folder.

	static Paths getPathsConfig(std::string const& confOption);  ///< Return expanded list of paths specified by a path config option
	static Paths const& getPaths();  ///< Get the data file search path
};

Paths getThemePaths();  ///< Get the data/theme file search path (includes current and default themes in addition to data folders)

std::string formatPath(const fs::path& target);

// Make std::filesystem::path formattable.
template <>
struct fmt::formatter<std::filesystem::path>: formatter<std::string_view>
{
	template <typename FormatContext>
	auto format(const std::filesystem::path& path, FormatContext& ctx) const 
	{
		return formatter<std::string_view>::format(formatPath(path), ctx);
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

using BinaryBuffer = std::vector<std::uint8_t>;

std::list<std::string> getThemes();  ///< Find all theme folders and return theme names.

/// Recursively copies a folder, throws on error.
void copyDirectoryRecursively(const fs::path& sourceDir, const fs::path& destinationDir);

fs::path execname(); ///< Get the path and filename of the main executable.

fs::path findFile(fs::path const& filename);  ///< Look for the specified file in theme and data folders.

BinaryBuffer readFile(fs::path const& path); ///< Reads a file into a buffer. 

Paths listFiles(fs::path const& dir);  ///< List contents of specified folder in theme and data folders (omit duplicates).

struct FsPathHash {
	size_t operator()(const fs::path& path) const noexcept {
		return fs::hash_value(path);
	}
};