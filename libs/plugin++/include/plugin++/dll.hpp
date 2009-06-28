#ifndef DLL_HPP_INCLUDED
#define DLL_HPP_INCLUDED

#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace plugin {
    namespace fs = boost::filesystem;

	/** @short RAII wrapper to load and unload dynamic library **/
    class dll {
        void* lib;
      public:
        dll(fs::path const& filename);
        ~dll();
    };

	/** @short A helper for loading all matching libraries in a folder. **/
    class loader {
        boost::ptr_vector<dll> dlls;
        void parse(std::set<fs::path>& paths, char const* var, fs::path const& folder = fs::path()) {
            if (!var) return;
            std::istringstream iss(var);
            std::string elem;
            while (std::getline(iss, elem, ':')) {
                fs::path p = elem;
                if (!folder.empty()) p /= folder;
                if (fs::is_directory(p)) paths.insert(p);
            }
        }
        /** Try to load all libraries in the given folder. **/
        void load(fs::path const& path) {
            for (fs::directory_iterator it(path), end; it != end; ++it) {
                try {
                    dlls.push_back(new dll(*it));
                } catch (std::runtime_error const& e) {
                    std::cerr << e.what() << std::endl;
                }
            }
        }
      public:
        loader(fs::path const& folder) {
            std::set<fs::path> paths;
            parse(paths, std::getenv("PLUGIN_PATH"));
            if (!folder.empty()) {
                parse(paths, std::getenv("LD_LIBRARY_PATH"), folder);
                parse(paths, "/usr/lib:/usr/local/lib", folder);
            }
            for (std::set<fs::path>::const_iterator it = paths.begin(); it != paths.end(); ++it) load(*it);
            if (dlls.empty()) std::cerr << "No plugins found. Try setting PLUGIN_PATH or LD_LIBRARY_PATH." << std::endl;
        }    
    };
}

#endif

