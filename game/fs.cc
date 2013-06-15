#include "fs.hh"

#include "config.hh"
#include "configuration.hh"
#include "execname.hh"
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <set>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

namespace {
	/// Test if a path begins with name and if so, remove that element and return true
	/// Mostly a workaround for fs::path's crippled API that makes this operation difficult
	bool pathRootHack(fs::path& p, std::string const& name) {
		if (p.empty() || p.begin()->string() != name) return false;
		fs::path ret;
		for (auto const& dir: p) {
			if (&dir == &*p.begin()) continue;  // Skip the first element
			ret /= dir;
		}
		p = ret;
		return true;
	}

	const fs::path performous = "performous";

	struct PathCache {
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

		fs::path base, home, locale, conf, data, cache;
		Paths paths;
		/// Initialize paths
		/// Note: Not done in constructor because logging and config are not initialized
		/// during static construction. Initialization must be done manually.
		void init() {
			std::string logmsg = "fs/info: Determining system paths:\n";
			// Base
			base = execname().parent_path().parent_path();
			logmsg += "  base:     " + base.string() + '\n';
			// Locale
			locale = fs::absolute(LOCALEDIR, base);
			logmsg += "  locale:   " + locale.string() + '\n';
			// Home
			{
			#ifdef _WIN32
				char const* p = getenv("USERPROFILE");
			#else
				char const* p = getenv("HOME");
			#endif
				if (p) home = p;
				logmsg += "  home:     " + home.string() + '\n';
			}
			// Config
			{
			#ifdef _WIN32
				ITEMIDLIST* pidl;
				HRESULT hRes = SHGetSpecialFolderLocation(nullptr, CSIDL_APPDATA|CSIDL_FLAG_CREATE, &pidl);
				if (hRes != NOERROR) throw std::runtime_error("Unable to determine where Application Data is stored");
				char p[MAX_PATH];
				SHGetPathFromIDList(pidl, p);
				conf = p;
			#else
				char const* p = getenv("XDG_CONFIG_HOME");
				conf = (p ? p : home / ".config");
			#endif
				conf /= performous;
				logmsg += "  config:   " + conf.string() + '\n';
			}
			// Data
			{
			#ifdef _WIN32
				data = config;
			#else
				char const* p = getenv("XDG_DATA_HOME");
				data = (p ? p / performous : home / ".local" / performous);
			#endif
				logmsg += "  data:     " + data.string() + '\n';
			}
			// Cache
			{
			#ifdef _WIN32
				cache = data / "cache";  // FIXME: Should we use GetTempPath?
			#else
				char const* p = getenv("XDG_CACHE_HOME");
				cache = (p ? p / performous : home / ".cache" / performous);
			#endif
				logmsg += "  cache:    " + cache.string() + '\n';
			}
			std::clog << logmsg << std::flush;
			// Data dirs
			logmsg = "fs/info: Determining data dirs (search path):\n";
			{
				const fs::path shareDir = SHARED_DATA_DIR;
				Paths dirs;
				dirs.push_back(data);  // Adding user's data dir
				dirs.push_back(base / shareDir);  // Adding relative path from executable
			#ifndef _WIN32
				// Adding XDG_DATA_DIRS
				{
					char const* xdg_data_dirs = getenv("XDG_DATA_DIRS");
					std::istringstream iss(xdg_data_dirs ? xdg_data_dirs : "/usr/local/share/:/usr/share/");
					for (std::string p; std::getline(iss, p, ':'); dirs.push_back(p / performous)) {}
				}
			#endif
				// Adding paths from config file
				auto const& conf = config["paths/system"].sl();
				for (std::string const& dir: conf) dirs.splice(dirs.end(), pathExpand(dir));
				// Check if they actually exist and print debug
				paths.clear();
				std::set<fs::path> used;
				for (auto dir: dirs) {
					dir = fs::absolute(dir);
					if (used.find(dir) != used.end()) continue;
					logmsg += "  " + dir.string() + '\n';
					paths.push_back(dir);
					used.insert(dir);
				}
			}
			std::clog << logmsg << std::flush;
		}
	} cache;
	std::mutex mutex;
	typedef std::lock_guard<std::mutex> Lock;
}

fs::path getHomeDir() { Lock l(mutex); return cache.home; }
fs::path getLocaleDir() { Lock l(mutex); return cache.home; }
fs::path getConfigDir() { Lock l(mutex); return cache.conf; }
fs::path getDataDir() { Lock l(mutex); return cache.data; }
fs::path getCacheDir() { Lock l(mutex); return cache.cache; }

Paths const& getPaths(bool refresh) {
	Lock l(mutex);
	if (refresh) cache.init();
	return cache.paths;
}

fs::path findFile(fs::path const& filename, Paths const& infixes = Paths(1)) {
	if (filename.empty()) throw std::logic_error("findFile expects a filename.");
	if (filename.is_absolute()) throw std::logic_error("findFile expects a filename without path.");
	Paths list;
	for (auto infix: infixes) {
		for (auto p: getPaths()) {
			p /= infix / filename;
			list.push_back(p);
			if (fs::exists(p)) return p.string();
		}
	}
	std::string logmsg = "fs/error: Unable to locate data file, tried:\n";
	for (auto const& p: list) logmsg += "  " + p.string() + '\n';
	std::clog << logmsg << std::flush;
	return fs::path();
}

fs::path getPath(fs::path const& filename) {
	fs::path file = findFile(filename);
	if (file.empty()) throw std::runtime_error("Cannot find file \"" + filename.string() + "\" in any of Performous data folders");
	return file;
}

fs::path getThemePath(fs::path const& filename) {
	const fs::path themes = "themes";
	const fs::path def = "default";
	std::string theme = config["game/theme"].getEnumName();
	// Try current theme and if that fails, try default theme and finally data dirs
	Paths infixes = { themes / def, fs::path() };
	if (!theme.empty() && theme != def) infixes.push_front(themes / theme);
	fs::path file = findFile(filename, infixes);
	if (file.empty()) throw std::runtime_error("Cannot find file \"" + filename.string() + "\" in Performous theme folders");
	return file;
}

std::vector<std::string> getThemes() {
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
	return std::vector<std::string>(themes.begin(), themes.end());
}

fs::path getDefaultConfig(fs::path const& configFile) {
	typedef std::vector<std::string> ConfigList;
	ConfigList config_list;
	char const* root = getenv("PERFORMOUS_ROOT");
	if (root) config_list.push_back(std::string(root) + "/" SHARED_DATA_DIR + configFile.string());
	fs::path exec = execname();
	if (!exec.empty()) config_list.push_back(exec.parent_path().string() + "/../" SHARED_DATA_DIR + configFile.string());
	ConfigList::const_iterator it = std::find_if(config_list.begin(), config_list.end(), static_cast<bool(&)(fs::path const&)>(fs::exists));
	if (it == config_list.end()) {
		throw std::runtime_error("Could not find default config file " + configFile.string());
	}
	return *it;
}

Paths getPathsConfig(std::string const& confOption) {
	Lock l(mutex);
	Paths ret;
	for (auto const& str: config[confOption].sl()) {
		ret.splice(ret.end(), cache.pathExpand(str));  // Add expanded paths to ret.
	}
	return ret;
}

void resetPaths() {
	Lock l(mutex);
	cache.init();
}

BinaryBuffer readFile(fs::path const& path) {
	BinaryBuffer ret;
	std::ifstream f(path.string(), std::ios::binary);
	f.seekg(0, std::ios::end);
	ret.resize(f.tellg());
	f.seekg(0);
	f.read(reinterpret_cast<char*>(ret.data()), ret.size());
	if (!f) throw std::runtime_error("File cannot be read: " + path.string());
	return ret;
}

