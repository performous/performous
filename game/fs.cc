#include "fs.hh"

#include "configuration.hh"
#include <cstdlib>
#include <iostream>

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
		return getXdgPath(fs::path("themes") / theme / filename);
	} else {
		if (*theme.rbegin() == '/') theme.erase(theme.size() - 1); // Remove trailing slash
		return theme + "/" + filename;
	}
}

std::string getXdgPath(fs::path const& filename) {
	static std::vector<fs::path> dir;
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		// Adding PERFORMOUS_DATA_DIR (compatibility)
		char const* env_data_dir = getenv("PERFORMOUS_DATA_DIR");
		if (env_data_dir) {
			dir.push_back(env_data_dir);
		}
		// Adding XDG_DATA_HOME
		char const* xdg_data_home = getenv("XDG_DATA_HOME");
		if (xdg_data_home) {
			dir.push_back(fs::path(xdg_data_home) / "performous");
		} else {
			dir.push_back(getHomeDir() / ".local" / "share" / "performous" );
		}
		// Adding XDG_DATA_DIRS
		char const* xdg_data_dirs = getenv("XDG_DATA_DIRS");
		if (xdg_data_dirs) {
			// here explode ":"
		} else {
			dir.push_back( "/usr/local/share/performous/" );
			dir.push_back( "/usr/share/performous/" );
		}
		// Adding some defaults
		std::vector<std::string> pathes = config["system/path"].sl();
		std::transform(pathes.begin(), pathes.end(), std::inserter(dir, dir.end()), pathMangle);
		// Some output
		for (std::vector<fs::path>::const_iterator it = dir.begin(); it != dir.end(); ++it) {
			std::cout << ">>> Adding XDG search path \"" << it->string() << "\"" << std::endl;
		}
	}
	for (std::vector<fs::path>::const_iterator it = dir.begin(); it != dir.end(); ++it) {
		fs::path p = *it;
		p /= filename;
		if( fs::exists(p) ) return p.string();
	}
	throw std::runtime_error("Cannot find file \"" + filename.string() + "\" inside XDG data pathes");
}
