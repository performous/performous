#include "fs.hh"

#include "config.hh"
#include "configuration.hh"
#include <plugin++/execname.hpp>
#include <cstdlib>
#include <iostream>
#include <list>
#include <sstream>

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

fs::path getConfigDir() {
	static fs::path dir;
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		char const* conf = getenv("XDG_CONFIG_HOME");
		if (conf) dir = fs::path(conf) / "performous";
		else dir = getHomeDir() / ".config" / "performous";
	}
	return dir;
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
	if (theme.empty()) throw std::runtime_error("Configuration value game/theme is empty");
	// Figure out theme folder (if theme name rather than path was given)
	if (theme.find('/') == std::string::npos) {
		return getPath(fs::path("themes") / theme / filename);
	} else {
		if (*theme.rbegin() == '/') theme.erase(theme.size() - 1); // Remove trailing slash
		return theme + "/" + filename;
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
	typedef std::list<fs::path> Dirs;
	static Dirs dirs;
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		fs::path shortDir = "performous";
		fs::path shareDir = SHARED_DATA_DIR;
#ifdef _WIN32
		// Add APPLIC~1 (user-specific application data)  FIXME: Not tested
		{
			char const* appdata = getenv("APPDATA");
			if (appdata) dirs.push_back(appdata / shortDir);
		}
#else
		// Adding XDG_DATA_HOME
		{
			char const* xdg_data_home = getenv("XDG_DATA_HOME");
			// FIXME: Should this use "games" or not?
			dirs.push_back(xdg_data_home ? xdg_data_home / shortDir : getHomeDir() / ".local" / shareDir);
		}
#endif
		// Adding relative path from executable
		dirs.push_back(fs::path(plugin::execname()).parent_path().parent_path() / shareDir);
#ifndef _WIN32
		// Adding XDG_DATA_DIRS
		{
			char const* xdg_data_dirs = getenv("XDG_DATA_DIRS");
			std::istringstream iss(xdg_data_dirs ? xdg_data_dirs : "/usr/local/share/:/usr/share/");
			for (std::string p; std::getline(iss, p, ':'); dirs.push_back(p / shortDir)) {}
		}
#endif
		// Adding paths from config file
		std::vector<std::string> pathes = config["system/path"].sl();
		std::transform(pathes.begin(), pathes.end(), std::inserter(dirs, dirs.end()), pathMangle);
		// Check if they actually exist and print debug
		dirs.remove_if(pathNotExist);
	}
	for (Dirs::const_iterator it = dirs.begin(); it != dirs.end(); ++it) {
		fs::path p = *it;
		p /= filename;
		if( fs::exists(p) ) return p.string();
	}
	throw std::runtime_error("Cannot find file \"" + filename.string() + "\" in any of Performous data folders");
}
