#include <plugin++/loader.hpp>
#include <plugin++/execname.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifdef _MSC_VER
#pragma warning(disable: 4996)  // Disable stupid Microsoft warnings about stdlib functions
#endif

using namespace plugin;
namespace fs = boost::filesystem;

#ifdef _WIN32
#define PATHSEP ';'
#else
#define PATHSEP ':'
#endif

/// Break var (a delimited path string) into paths (appends folder if supplied).
void loader::parse(std::set<fs::path>& paths, char const* var, fs::path const& folder) {
    if (!var) return;
    std::istringstream iss(var);
    std::string elem;
    while (std::getline(iss, elem, PATHSEP)) {
        fs::path p = elem;
        if (!folder.empty()) p /= folder;
        if (fs::is_directory(p)) paths.insert(p);
    }
}

/// Try to load all libraries in the given folder.
void loader::load(fs::path const& path) {
    for (fs::directory_iterator it(path), end; it != end; ++it) {
        try {
            dlls.push_back(new dll(it->string()));
        } catch (std::runtime_error const& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

loader::loader(fs::path const& folder) {
    std::set<fs::path> paths;
    // Try PLUGIN_PATH environment setting
    parse(paths, std::getenv("PLUGIN_PATH"));
    // Try other options only if folder was given
    if (!folder.empty()) {
		// Try relative path from executable
		fs::path p = execname();
		p = p.parent_path().parent_path();
		if (!p.empty()) {
			p /= "lib" / folder;
			if (fs::is_directory(p)) paths.insert(p);
		}
#ifndef _WIN32
	    // Try UNIX library paths
        parse(paths, std::getenv("LD_LIBRARY_PATH"), folder);
        parse(paths, "/usr/lib:/usr/local/lib", folder);
#endif
    }
    // Load the contents of each folder
    for (std::set<fs::path>::const_iterator it = paths.begin(); it != paths.end(); ++it) load(*it);
    if (dlls.empty()) std::cerr << "No plugins found. Try setting PLUGIN_PATH to the folder with the plugins." << std::endl;
}    

