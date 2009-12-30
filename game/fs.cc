#include "fs.hh"

#include "config.hh"
#include "configuration.hh"
#include <plugin++/execname.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <Shlobj.h>
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
#ifdef _WIN32
		// Add APPLIC~1 (user-specific application data)  FIXME: Not tested
		{
			//char const* appdata = getenv("APPDATA");
			//if (appdata) dirs.push_back(appdata / shortDir);
			dirs.push_back(getConfigDir());
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
	}
	return paths;
}

