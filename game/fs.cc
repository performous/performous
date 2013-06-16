#include "fs.hh"

#include "config.hh"
#include "configuration.hh"
#include "execname.hh"
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <cstdlib>
#include <iostream>
#include <boost/thread.hpp>
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
	const fs::path configSchema = "config/schema.xml";

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

		fs::path base, share, locale, sysConf, home, conf, data, cache;
		Paths paths;
		// Note: three-phase init:
		// 1. Default constructor runs in static context (before main) and cannot do much
		// 2. pathBootstrap is called to find out static system paths, and enable loading of config files
		// 3. pathInit is called to process the full search path, using config settings
		/// Initialize static system paths, returns config schema path.
		fs::path pathBootstrap() {
			std::string logmsg = "fs/info: Determining system paths:\n";
			// Base (e.g. /usr/local), share (src or installed data files) and locale (built or installed .mo files)
			{
				char const* root = getenv("PERFORMOUS_ROOT");
				base = fs::absolute(root ? root : execname().parent_path());
				do {
					if (base.empty()) throw std::runtime_error("Unable to find Performous data files. Install properly or set environment variable PERFORMOUS_ROOT.");
					for (fs::path const& infix: { fs::path(SHARED_DATA_DIR), fs::path("data"), fs::path() }) {
						if (!fs::exists(base / infix / configSchema)) continue;
						share = base / infix;
						goto found;
					}
					// Use locale .mo files from build folder?
					if (base.filename() == "build" && fs::exists(base / "lang")) {
						locale = base / "lang";
					}
					base = base.parent_path();
				} while (true);
			found:;
				if (locale.empty() && fs::exists(base / LOCALEDIR)) locale = base / LOCALEDIR;
				logmsg += "  base:     " + base.string() + '\n';
				logmsg += "  share:    " + share.string() + '\n';
				logmsg += "  locale:   " + locale.string() + '\n';
			}
			// System-wide config files
			{
			#ifdef _WIN32
				sysConf = execname().parent_path() / "config";  // I.e. Program Files/Performous/config or build/config/
			#else
				sysConf = "/etc/xdg/performous";
			#endif
				logmsg += "  sysConf:  " + sysConf.string() + '\n';
			}
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
				data = conf;
			#else
				char const* p = getenv("XDG_DATA_HOME");
				data = (p ? p / performous : home / ".local" / SHARED_DATA_DIR);
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
			pathInit();
			return share / configSchema;
		}
		/// Initialize/reset data dirs (search path).
		void pathInit() {
			bool bootstrapping = paths.empty();  // The first run (during bootstrap)
			// Data dirs
			std::string logmsg = "fs/info: Determining data dirs (search path):\n";
			{
				Paths dirs;
				dirs.push_back(data);  // Adding user's data dir
				dirs.push_back(share);  // Adding system data dir (relative to performous executable or PERFORMOUS_ROOT)
			#ifndef _WIN32
				// Adding XDG_DATA_DIRS
				{
					char const* xdg_data_dirs = getenv("XDG_DATA_DIRS");
					std::istringstream iss(xdg_data_dirs ? xdg_data_dirs : "/usr/local/share/:/usr/share/");
					for (std::string p; std::getline(iss, p, ':'); dirs.push_back(p / performous)) {}
				}
			#endif
				// Adding paths from config file (during bootstrap config options are not yet available)
				if (!bootstrapping) {
					auto const& conf = config["paths/system"].sl();
					for (std::string const& dir: conf) dirs.splice(dirs.end(), pathExpand(dir));
				}
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
			if (!bootstrapping) std::clog << logmsg << std::flush;
		}
	} cache;
	boost::mutex mutex;
	typedef boost::lock_guard<boost::mutex> Lock;
}

fs::path pathBootstrap() { Lock l(mutex); return cache.pathBootstrap(); }
void pathInit() { Lock l(mutex); cache.pathInit(); }
fs::path getHomeDir() { Lock l(mutex); return cache.home; }
fs::path getShareDir() { Lock l(mutex); return cache.share; }
fs::path getLocaleDir() { Lock l(mutex); return cache.locale; }
fs::path getConfigDir() { Lock l(mutex); return cache.conf; }
fs::path getSysConfigDir() { Lock l(mutex); return cache.sysConf; }
fs::path getDataDir() { Lock l(mutex); return cache.data; }
fs::path getCacheDir() { Lock l(mutex); return cache.cache; }
Paths const& getPaths() { Lock l(mutex); return cache.paths; }

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

Paths getPathsConfig(std::string const& confOption) {
	Lock l(mutex);
	Paths ret;
	for (auto const& str: config[confOption].sl()) {
		ret.splice(ret.end(), cache.pathExpand(str));  // Add expanded paths to ret.
	}
	return ret;
}

BinaryBuffer readFile(fs::path const& path) {
	BinaryBuffer ret;
	fs::ifstream f(path, std::ios::binary);
	f.seekg(0, std::ios::end);
	ret.resize(f.tellg());
	f.seekg(0);
	f.read(reinterpret_cast<char*>(ret.data()), ret.size());
	if (!f) throw std::runtime_error("File cannot be read: " + path.string());
	return ret;
}

