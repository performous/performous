#include "fs.hh"

#include "configuration.hh"
#include "log.hh"
#include "platform.hh"
#include "util.hh"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <set>
#include <sstream>
#include <regex>

#include <boost/range.hpp>

#if (BOOST_OS_WINDOWS)
#include <shlobj.h>
#endif

/// Test if a path begins with name and if so, remove that element and return true
/// Mostly a workaround for fs::path's crippled API that makes this operation difficult
bool PathCache::pathRootHack(fs::path& p, std::string const& name) {
	if (p.empty() || p.begin()->string() != name) return false;
	fs::path ret;
	// Add all but first element
	for (auto dir = ++p.begin(); dir != p.end(); ++dir)
		ret /= *dir;
	p = ret;
	return true;
}


void PathCache::pathBootstrap() {
	if (!base.empty()) return;  // Only bootstrap once
	// Base (e.g. /usr/local), share (src or installed data files) and locale (built or installed .mo files)
	{
		char const* root = getenv("PERFORMOUS_ROOT");
		switch (Platform::currentOS()) {
			case Platform::HostOS::OS_WIN:
				base = fs::canonical(root ? root : execname().parent_path());
				break;
			case Platform::HostOS::OS_MAC:
				if (const auto appFolder{execname().parent_path().parent_path().parent_path()}; appFolder.extension() == ".app") {
					base = fs::canonical(appFolder);
					break;
				}
				[[fallthrough]]; // if not running from .app bundle, handle like regular unix.
			default:
				base = fs::canonical(root ? root : execname().parent_path().parent_path());
				break;
		}
		std::set<fs::path> infixPaths{fs::path(SHARED_DATA_DIR), fs::path("data"),fs::path()};
		do {
			if (base.empty()) throw std::runtime_error("Unable to find Performous data files. Install properly or set environment variable PERFORMOUS_ROOT.");
			for (auto const& infix : infixPaths) {
				if (!fs::exists(base / infix / PathCache::CONFIG_SCHEMA)) continue;
				share = base / infix;
				break; // Found
			}
			if (!share.empty()) {
				break; // Found
			}
			base = base.parent_path();
		} while (true);
		// Use locale .mo files from build folder?
		if (Platform::currentOS() == Platform::HostOS::OS_WIN) {
			auto folder = base.filename();
			auto reg = "x\\d{2}-([Dd]ebug|[Rr]elease)(-install)?"; // matches all build folders.
			if ((folder == "build" || std::regex_search(folder.string(), std::regex(reg))) && fs::exists(base / "lang")) {
				locale = base / "lang";
			}
		}
		else {
			if (base.filename() == "build" && fs::exists(base / "lang")) {
				locale = base / "lang";
			}
		}
		if (locale.empty() && fs::exists(base / LOCALE_DIR)) locale = base / LOCALE_DIR;
	}
	// System-wide config files
	{
		switch (Platform::currentOS()) {
			case Platform::HostOS::OS_MAC:
				sysConf = "/Library/Preferences/Performous";
				break;
			case Platform::HostOS::OS_WIN:
				sysConf = execname().parent_path() / "config";
				break;
			default:
				sysConf = "/etc/xdg/performous";
		}
	}

	// Home
	{
	char const* p = Platform::currentOS() == Platform::HostOS::OS_WIN ? getenv("USERPROFILE") : getenv("HOME");
		if (p) home = p;
	}
	// Config
	{
#if (BOOST_OS_WINDOWS)
		PWSTR p;
		HRESULT hRes = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &p);
		if (hRes != NOERROR) throw std::runtime_error("Unable to determine where Application Data is stored");
		conf = p;
		conf /= PathCache::PERFORMOUS;
#else
		if (Platform::currentOS() == Platform::HostOS::OS_MAC) {
			conf = (home / "Library/Preferences/Performous");
		}
		else {
			char const* p = getenv("XDG_CONFIG_HOME");
			conf = (p ? p : home / ".config");
			conf /= PathCache::PERFORMOUS_PATH;
		}
#endif
	}

	// Data
	{
		switch (Platform::currentOS()) {
			case Platform::HostOS::OS_MAC:
				data = share;
				break;
			case Platform::HostOS::OS_WIN:
				data = conf;
				break;
			default:
				char const* p = getenv("XDG_DATA_HOME");
				data = (p ? p / PathCache::PERFORMOUS_PATH : home / ".local" / SHARED_DATA_DIR);
		}
	}
	// Cache
	{
		switch (Platform::currentOS()) {
			case Platform::HostOS::OS_MAC:
				cache = (home / "Library/Caches/Performous");
				break;
			case Platform::HostOS::OS_WIN:
				cache = data / "cache";
				break;
			default:
				char const* p = getenv("XDG_DATA_HOME");
				cache = (p ? p / PathCache::PERFORMOUS_PATH : home / ".cache" / PathCache::PERFORMOUS_PATH);
		}
	}
	pathInit();
}

void PathCache::pathInit() {
	bool bootstrapping = paths.empty();  // The first run (during bootstrap)
	if (!bootstrapping) {
		std::string logmsg{fmt::format(
		"Found system paths:\n"
		"{8}base:          {0}\n"
		"{8}share:         {1}\n"
		"{8}locale:        {2}\n"
		"{8}sysConf:       {3}\n"
		"{8}home           {4}\n"
		"{8}config:        {5}\n"
		"{8}data:          {6}\n"
		"{8}cache:         {7}",
		base, share, locale, sysConf, home, conf, data, cache, SpdLogger::newLineDec
		)};
		SpdLogger::info(LogSystem::FILESYSTEM, logmsg);
	}

	if (Platform::currentOS() == Platform::HostOS::OS_MAC) {
	char const* p = getenv("XDG_CONFIG_HOME");
	fs::path oldConf = (p ? p : home / ".config/performous");
	if (fs::is_directory(oldConf)) {
		SpdLogger::info(LogSystem::FILESYSTEM, "Configuration files found in old location, path={}", oldConf);
		conf = home / "Library" / "Preferences" / "Performous";
		if (bootstrapping) {
			copyDirectoryRecursively(oldConf, conf);
			try {
				fs::remove_all(oldConf);
				fs::path oldCache = (home / ".cache/performous");
				fs::remove_all(oldCache);
				didMigrateConfig = true;
			}
			catch (fs::filesystem_error const& e) {
				throw std::runtime_error(fmt::format("There was an error migrating configuration to path={}. Exception={}", conf, e.what()));
			}
		}
	}
	if (didMigrateConfig) {
		SpdLogger::info(LogSystem::FILESYSTEM, "Successfully moved configuration files to their new location, path={}", conf);
		}
	}

	// Data dirs
	std::string logmsg{"Determining data dirs (search path):"};
	{
		Paths dirs;
		dirs.push_back(data);  // Adding user's data dir
		dirs.push_back(share);  // Adding system data dir (relative to performous executable or PERFORMOUS_ROOT)
		if (Platform::currentOS() != Platform::HostOS::OS_WIN) {
			// Adding XDG_DATA_DIRS
			{
				char const* xdg_data_dirs = getenv("XDG_DATA_DIRS");
				std::istringstream iss(xdg_data_dirs ? xdg_data_dirs : "/usr/local/share/:/usr/share/");
				for (std::string p; std::getline(iss, p, ':'); dirs.push_back(p / PathCache::PERFORMOUS_PATH)) {}
			}
		}
		// Adding paths from config file (during bootstrap config options are not yet available)
		if (!bootstrapping) {
			auto const& conf = config["paths/system"].sl();
			for (std::string const& dir: conf) dirs.splice(dirs.end(), pathExpand(dir));
		}
		// Check if they actually exist and print debug
		paths.clear();
		std::set<fs::path> used;
		for (auto dir: dirs) {
			dir = fs::weakly_canonical(dir);
			if (used.find(dir) != used.end()) continue;
			fmt::format_to(std::back_inserter(logmsg), "\n{}{}", SpdLogger::newLineDec, dir);
			paths.push_back(dir);
			used.insert(dir);
		}
	}
	if (!bootstrapping) {
		SpdLogger::info(LogSystem::FILESYSTEM, logmsg);
	}
}

Paths PathCache::pathExpand(fs::path p) {
	Paths ret;
	if (pathRootHack(p, "~")) ret.push_back(home / p);
	else if (pathRootHack(p, "DATADIR")) {
		// Add all data paths with p appended to them
		for (auto const& path: paths) ret.push_back(path / p);
	}
	else ret.push_back(p);
	return ret;
}


void PathCache::copyDirectoryRecursively(const fs::path& sourceDir, const fs::path& destinationDir)
{
	if (!fs::exists(sourceDir) || !fs::is_directory(sourceDir)) {
		throw std::runtime_error("Source directory " + sourceDir.string() + " does not exist or is not a directory");
	}
	if (!fs::create_directory(destinationDir) && !fs::exists(destinationDir)) {
		throw std::runtime_error("Cannot create destination directory " + destinationDir.string());
	}
	for (const auto& dirEnt : fs::recursive_directory_iterator{sourceDir}) {
		const auto& path = dirEnt.path();
		auto relativePathStr = replaceFirst(path.string(), sourceDir.string(), "");

		try {
			if (!fs::is_directory(path)) {
				fs::copy_file(path, destinationDir / relativePathStr);
			}
			else {
				create_directory(destinationDir / relativePathStr, path);
			}
		} catch (...) {
			throw std::runtime_error("Cannot copy file " + path.string() + ", because it already exists in the destination folder.");
		}
	}
}


fs::path PathCache::getCanonicalPath( const fs::path &target ) {
	if ( target.has_parent_path() ) {
		return fs::weakly_canonical(target);
	}
	return target;
}

std::string PathCache::formatPath(const fs::path& target) {

	if (target.empty()) {
		return "<blank>";
	}
	// Normalize the paths by resolving symlinks and removing redundant elements.
	// But, if just a filename, keep it as is.
	static fs::path canonicalTarget = getCanonicalPath( target );
	fs::path baseDir = getBaseDir();
	if (canonicalTarget == baseDir) {
	// Return the absolute path if referring to the base directory.
		return baseDir.string();
	}
	// Check if the target is a descendant of base.
	if (std::mismatch(baseDir.begin(), baseDir.end(), canonicalTarget.begin()).first == baseDir.end()) {
		// If target is a descendant of base, return the relative path starting from base's parent.
		return fs::relative(canonicalTarget, baseDir.parent_path()).string();
	} else {
		// Otherwise, return the absolute path to target.
		return canonicalTarget.string();
	}
}

fs::path PathCache::getProfilerLogFilename() { 
	Lock l(mutex);
	std::string baseName{"profiler.txt"};
	unsigned logRotate = 5;
	do {
		fs::path newPath = cache / fmt::format("profiler.{}.txt", logRotate);
		fs::path oldPath = cache / (logRotate == 1 ? "profiler.txt" : fmt::format(fmt::runtime("profiler.{}.txt"), logRotate - 1));
		if (fs::exists(oldPath)) {
			if (logRotate == 5 && fs::exists(newPath)) {
				fs::remove(newPath); // delete profiler.5.txt if it exists
			}
			fs::rename(oldPath, newPath); // rotate
		}
		logRotate--;
	}
	while (logRotate >= 1);
	return cache / "profiler.txt";  //TODO - make a named constant?
}


Paths PathCache::getThemePaths() {
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

fs::path PathCache::findFile(fs::path const& filename) {
	if (filename.empty()) throw std::logic_error("findFile expects a filename.");
	if (filename.is_absolute()) throw std::logic_error("findFile expects a filename without path.");
	Paths list;
	for (fs::path p: getThemePaths()) {
		p /= filename;
		list.push_back(p);
		if (fs::exists(p)) return p.string();
	}
	std::string logmsg{"Unable to locate data file, tried:"};
	for (auto const& p: list) fmt::format_to(std::back_inserter(logmsg), "\n{}", p);
	SpdLogger::error(LogSystem::FILESYSTEM, logmsg);
	throw std::runtime_error("Cannot find file \"" + filename.string() + "\" in Performous theme or data folders");
}

Paths PathCache::listFiles(fs::path const& dir) {
	if (dir.is_absolute()) throw std::logic_error("listFiles expects a folder name without path.");
	std::set<fs::path> found; // Filenames already found
	Paths files; // Full paths of files found
	for (fs::path &path: getThemePaths()) {
		fs::path subdir = path / dir;
		if (!fs::is_directory(subdir))
			continue;
		for (const auto &d : fs::recursive_directory_iterator(subdir)) {
			fs::path name = d.path().filename(); // FIXME: Extract full path from current folder, not just the filename
			// If successfully inserted to "found", it wasn't found before, so add to paths.
			if (found.insert(name).second) files.push_back(d);
		}
	}
	return files;
}

std::list<std::string> PathCache::getThemes() {
	std::set<std::string> themes;
	// Search all paths for themes folders and add them
	for (auto p: getPaths()) {
		p /= "themes";
		if (!fs::is_directory(p)) continue;
		// Gather the themes in this folder
		for (const auto &dir : fs::directory_iterator(p)) {
			fs::path p2 = dir.path();
			if (fs::is_directory(p2)) themes.insert(p2.filename().string());
		}
	}
	return std::list<std::string>(themes.begin(), themes.end());
}

Paths PathCache::getPathsConfig(std::string const& confOption) {
	Lock l(mutex);
	Paths ret;
	for (auto const& str: config[confOption].sl()) {
		ret.splice(ret.end(), pathExpand(str)); // Add expanded paths to ret.
	}
	return ret;
}



/**
  * \brief     Helper function to read an entire binary file
  * \param[in] path[in]  Full path to the file we're reading
  * \returns   BinaryBuffer of bytes
  */
BinaryBuffer readFile(fs::path const& path) {
	BinaryBuffer ret;
	fs::ifstream f(path, std::ios::binary);
	f.seekg(0, std::ios::end);
	ret.resize(static_cast<size_t>(f.tellg()));
	f.seekg(0);
	f.read(reinterpret_cast<char*>(ret.data()), static_cast<std::streamsize>(ret.size()));
	if (!f) throw std::runtime_error("File cannot be read: " + path.string());
	return ret;
}

