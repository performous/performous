#include "fs.hh"

#include "configuration.hh"
#include <cstdlib>

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
		ConfigItem::StringList sd = config["system/path_themes"].sl();
		for (std::vector<std::string>::const_iterator it = sd.begin(); it != sd.end(); ++it) {
			fs::path p = *it;
			p /= theme;
			if (fs::is_directory(p)) { theme = p.string(); break; }
		}
    }
	if (*theme.rbegin() == '/') theme.erase(theme.size() - 1); // Remove trailing slash
	return theme + "/" + filename;
}

std::string getDataPath(std::string const& filename) {
	char const* env_data_dir = getenv("PERFORMOUS_DATA_DIR");
	if (env_data_dir) {
		return std::string(env_data_dir)+filename;
	} else {
		return std::string(filename);
	}
}
