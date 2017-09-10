#include "config.hh"
#include "configuration.hh"
#include "fs.hh"
#include "platform.hh"

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/range.hpp>
#include <boost/thread.hpp>
#include <cstdlib>

#include <set>
#include <sstream>
#include <algorithm>

#if (BOOST_OS_WINDOWS)
#include <windows.h>
#include <shlobj.h>
#endif

namespace {
	/// Test if a path begins with name and if so, remove that element and return true
	/// Mostly a workaround for fs::path's crippled API that makes this operation difficult
	bool pathRootHack(fs::path& p, std::string const& name) {
		if (p.empty() || p.begin()->string() != name) return false;
		fs::path ret;
		// Add all but first element
		for (auto dir = ++p.begin(); dir != p.end(); ++dir)
			ret /= *dir;
		p = ret;
		return true;
	}

	const fs::path performous = "performous";
	const fs::path configSchema = "config/schema.xml";	

	struct PathCache {
		Paths paths;
		bool didMigrateConfig = false;
		fs::path base, share, locale, sysConf, home, conf, data, cache;
		/// Expand a path specifier as a list of actual paths. Expands ~ (home) and DATADIR (Performous search path).
	Paths pathExpand(fs::path p) {
			Paths ret;
			if (pathRootHack(p, "~")) ret.push_back(home / p);
			else if (pathRootHack(p, "DATADIR")) {
				// Add all data paths with p appended to them
				for (auto const& path: paths) ret.push_back(path / p);
			}
			else ret.push_back(p);
			return ret;
		}
		// Note: three-phase init:
		// 1. Default constructor runs in static context (before main) and cannot do much
		// 2. pathBootstrap is called to find out static system paths (critical for logging and for loading config files)
		// 3. pathInit is called to process the full search path, using config settings
		
	#if (BOOST_OS_WINDOWS)
	#include "platform/fs_paths.win.inc"
	#else
	#include "platform/fs_paths.unix.inc"
	#endif
	} cache;

	boost::mutex mutex;
	typedef boost::lock_guard<boost::mutex> Lock;
}

void copyDirectoryRecursively(const fs::path& sourceDir, const fs::path& destinationDir)
{
    if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir))
    {
        throw std::runtime_error("Source directory " + sourceDir.string() + " does not exist or is not a directory");
    }
    if (!fs::create_directory(destinationDir) && !fs::exists(destinationDir))
    {
        throw std::runtime_error("Cannot create destination directory " + destinationDir.string());
    }
#if ((BOOST_VERSION / 100 % 1000) >= 55)
    for (const auto& dirEnt : fs::recursive_directory_iterator{sourceDir})
#else
    for (fs::recursive_directory_iterator dirEnt(sourceDir); dirEnt !=fs::recursive_directory_iterator(); ++dirEnt)
#endif
    {
    #if ((BOOST_VERSION / 100 % 1000) >= 55)
        const auto& path = dirEnt.path();
    #else
        const auto& path = dirEnt->path();    
    #endif
        auto relativePathStr = path.string();
        boost::algorithm::replace_first(relativePathStr, sourceDir.string(), "");
        try { 
        if (!fs::is_directory(path)) { fs::copy_file(path, destinationDir / relativePathStr); }
        else { fs::copy_directory(path, destinationDir / relativePathStr); }
        }
        catch (...) {
        throw std::runtime_error("Cannot copy file " + path.string() + ", because it already exists in the destination folder.");
        }
    }
}

void pathBootstrap() { Lock l(mutex); cache.pathBootstrap(); }
void pathInit() { Lock l(mutex); cache.pathInit(); }
fs::path getLogFilename() { Lock l(mutex); return cache.cache / "infolog.txt"; }
fs::path getSchemaFilename() { Lock l(mutex); return cache.share / configSchema; }
fs::path getHomeDir() { Lock l(mutex); return cache.home; }
fs::path getShareDir() { Lock l(mutex); return cache.share; }
fs::path getLocaleDir() { Lock l(mutex); return cache.locale; }
fs::path getConfigDir() { Lock l(mutex); return cache.conf; }
fs::path getSysConfigDir() { Lock l(mutex); return cache.sysConf; }
fs::path getDataDir() { Lock l(mutex); return cache.data; }
fs::path getCacheDir() { Lock l(mutex); return cache.cache; }
Paths const& getPaths() { Lock l(mutex); return cache.paths; }

Paths getThemePaths() {
	const fs::path themes = "themes";
	const fs::path def = "default";
	const fs::path www = "www";
	const fs::path js = "js";
	const fs::path css = "css";
	const fs::path images = "images";
	const fs::path fonts = "fonts";

	std::string theme = config["game/theme"].getEnumName();
	Paths paths = getPaths();
	Paths infixes = { 
					  themes / theme,
					  themes / theme / www,
					  themes / theme / www / js,
					  themes / theme / www / css,
					  themes / theme / www / images,
					  themes / theme / www / fonts,

					  themes / def,
					  themes / def / www,					  
					  themes / def / www / js,
					  themes / def / www / css,				  
					  themes / def / www / images,			  
					  themes / def / www / fonts,
					  fs::path() };
	if (!theme.empty() && theme != def) infixes.push_front(themes / theme);
	// Build combinations of paths and infixes
	Paths themePaths;
	for (fs::path const& infix: infixes) {
		for (fs::path p: paths) {
			p /= infix;
			if (fs::is_directory(p)) themePaths.push_back(p);
		}
	}
	return themePaths;
}

fs::path findFile(fs::path const& filename) {
	if (filename.empty()) throw std::logic_error("findFile expects a filename.");
	if (filename.is_absolute()) throw std::logic_error("findFile expects a filename without path.");
	Paths list;
	for (fs::path p: getThemePaths()) {
		p /= filename;
		list.push_back(p);
		if (fs::exists(p)) return p.string();
	}
	std::string logmsg = "fs/error: Unable to locate data file, tried:\n";
	for (auto const& p: list) logmsg += "  " + p.string() + '\n';
	std::clog << logmsg << std::flush;
	throw std::runtime_error("Cannot find file \"" + filename.string() + "\" in Performous theme or data folders");
}

Paths listFiles(fs::path const& dir) {
	if (dir.is_absolute()) throw std::logic_error("listFiles expects a folder name without path.");
	std::set<fs::path> found;  // Filenames already found
	Paths files;  // Full paths of files found
	for (fs::path path: getThemePaths()) {
		fs::path subdir = path / dir;
		if (!fs::is_directory(subdir))
			continue;
		for (fs::recursive_directory_iterator dirIt(subdir), dirEnd; dirIt != dirEnd; ++dirIt) {
			fs::path name = dirIt->path().filename();  // FIXME: Extract full path from current folder, not just the filename
			// If successfully inserted to "found", it wasn't found before, so add to paths.
			if (found.insert(name).second) files.push_back(*dirIt);
		}
	}
	return files;
}

std::list<std::string> getThemes() {
	std::set<std::string> themes;
	// Search all paths for themes folders and add them
	for (auto p: getPaths()) {
		p /= "themes";
		if (!fs::is_directory(p)) continue;
		// Gather the themes in this folder
		for (fs::directory_iterator dirIt(p), dirEnd; dirIt != dirEnd; ++dirIt) {
			fs::path p2 = dirIt->path();
			if (fs::is_directory(p2)) themes.insert(p2.filename().string());
		}
	}
	return std::list<std::string>(themes.begin(), themes.end());
}

Paths getPathsConfig(std::string const& confOption) {
	Lock l(mutex);
	Paths ret;
	for (auto const& str: config[confOption].sl()) {
		ret.splice(ret.end(), cache.pathExpand(str));  // Add expanded paths to ret.
	}
	return ret;
}
