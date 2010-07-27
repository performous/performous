#include "fs.hh"

#include "config.hh"
#include "configuration.hh"
#include <plugin++/execname.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

fs::path getHomeDir() {
	static fs::path dir;
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		char const* home = getenv("HOME");
		if (home) dir = home;
	}
	return dir;
}

fs::path getLocaleDir() {
	static fs::path dir;

	if(LOCALEDIR[0] == '/') {
		dir = LOCALEDIR;
	} else {
		dir = plugin::execname().parent_path().parent_path() / LOCALEDIR;
	}

	return dir;
}

fs::path getConfigDir() {
	static fs::path dir;
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		#ifndef _WIN32
		{
			char const* conf = getenv("XDG_CONFIG_HOME");
			if (conf) dir = fs::path(conf) / "performous";
			else dir = getHomeDir() / ".config" / "performous";
		}
		#else
		{
			//open AppData directory
			std::string str;
			ITEMIDLIST* pidl;
			char AppDir[MAX_PATH];
			HRESULT hRes = SHGetSpecialFolderLocation( NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE , &pidl );
			if (hRes==NOERROR)
			{
			  SHGetPathFromIDList( pidl, AppDir );
			  int i;
			  for(i = 0; AppDir[i] != '\0'; i++){
				  if(AppDir[i] == '\\') str += '/';
				  else                  str += AppDir[i];
			  }
			  dir = fs::path(str) / "performous";
			}
		}
		#endif
	}
	return dir;
}

fs::path getDataDir() {
#ifdef _WIN32
		return getConfigDir();  // APPDATA/performous
#else
		fs::path shortDir = "performous";
		fs::path shareDir = SHARED_DATA_DIR;
		char const* xdg_data_home = getenv("XDG_DATA_HOME");
		// FIXME: Should this use "games" or not?
		return (xdg_data_home ? xdg_data_home / shortDir : getHomeDir() / ".local" / shareDir);
#endif
}

fs::path getCacheDir() {
#ifdef _WIN32
		return getConfigDir() / "cache";  // APPDATA/performous
#else
		fs::path shortDir = "performous";
		char const* xdg_cache_home = getenv("XDG_CACHE_HOME");
		// FIXME: Should this use "games" or not?
		return (xdg_cache_home ? xdg_cache_home / shortDir : getHomeDir() / ".cache" / shortDir);
#endif
}

fs::path getThemeDir() {
	std::string theme = config["game/theme"].s();
	static const std::string defaultTheme = "default";
	if (theme.empty()) theme = defaultTheme;
	return getDataDir() / "themes" / theme;
}


fs::path pathMangle(fs::path const& dir) {
	fs::path ret;
	for (fs::path::const_iterator it = dir.begin(); it != dir.end(); ++it) {
		if (it == dir.begin() && *it == "~") {
			ret = getHomeDir();
			continue;
		}
		ret /= *it;
	}
	return ret;
}

std::string getThemePath(std::string const& filename) {
	std::string theme = config["game/theme"].s();
	static const std::string defaultTheme = "default";
	if (theme.empty()) theme = defaultTheme;
	// Try current theme and if that fails, try default theme.
	try {
		return getPath(fs::path("themes") / theme / filename);
	} catch (std::runtime_error&) {
		if (theme == defaultTheme) throw;
		return getPath(fs::path("themes") / defaultTheme / filename);
	}
}

namespace {
	bool pathNotExist(fs::path const& p) {
		if (exists(p)) {
			std::cout << ">>> Using data path \"" << p.string() << "\"" << std::endl;
			return false;
		}
		std::cout << ">>> Not using \"" << p.string() << "\" (does not exist)" << std::endl;
		return true;
	}
}

std::string getPath(fs::path const& filename) {
	Paths const& paths = getPaths();
	for (Paths::const_iterator it = paths.begin(); it != paths.end(); ++it) {
		fs::path p = *it;
		p /= filename;
		if( fs::exists(p) ) return p.string();
	}
	throw std::runtime_error("Cannot find file \"" + filename.string() + "\" in any of Performous data folders");
}

Paths const& getPaths(bool refresh) {
	static Paths paths;
	static bool initialized = false;
	if (!initialized || refresh) {
		initialized = true;
		fs::path shortDir = "performous";
		fs::path shareDir = SHARED_DATA_DIR;
		Paths dirs;

		// Adding users data dir
		dirs.push_back(getDataDir());

		// Adding relative path from executable
		dirs.push_back(plugin::execname().parent_path().parent_path() / shareDir);
#ifndef _WIN32
		// Adding XDG_DATA_DIRS
		{
			char const* xdg_data_dirs = getenv("XDG_DATA_DIRS");
			std::istringstream iss(xdg_data_dirs ? xdg_data_dirs : "/usr/local/share/:/usr/share/");
			for (std::string p; std::getline(iss, p, ':'); dirs.push_back(p / shortDir)) {}
		}
#endif
		// Adding paths from config file
		std::vector<std::string> const& confPaths = config["system/path"].sl();
		std::transform(confPaths.begin(), confPaths.end(), std::inserter(dirs, dirs.end()), pathMangle);
		// Check if they actually exist and print debug
		paths.clear();
		std::remove_copy_if(dirs.begin(), dirs.end(), std::inserter(paths, paths.end()), pathNotExist);
		// Assure that each path appears only once
		Paths::iterator it = std::unique(paths.begin(), paths.end());
		paths.resize(it - paths.begin());
	}
	return paths;
}

Paths getPathsConfig(std::string const& confOption) {
	Paths ret;
	Paths const& paths = getPaths();
	ConfigItem::StringList const& sl = config[confOption].sl();
	for (ConfigItem::StringList::const_iterator it = sl.begin(), end = sl.end(); it != end; ++it) {
		fs::path p = pathMangle(*it);
		if (p.empty()) continue;  // Ignore empty paths
		if (*p.begin() == "DATADIR") {
			// Remove first element
			p = p.string().substr(7);
			// Add all data paths with p appended to them
			for (Paths::const_iterator it2 = paths.begin(), end2 = paths.end(); it2 != end2; ++it2) {
				ret.push_back(*it2 / p);
			}
			continue;
		}
		ret.push_back(p);
	}
	return ret;
}

