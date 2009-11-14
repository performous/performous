#ifndef LOADER_HPP_INCLUDED
#define LOADER_HPP_INCLUDED

#include "dll.hpp"
#include <set>
#include <boost/filesystem.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace plugin {
    namespace fs = boost::filesystem;

    /** @short A helper for loading all matching libraries in a folder. **/
    class loader {
        boost::ptr_vector<dll> dlls;
        void parse(std::set<fs::path>& paths, char const* var, fs::path const& folder = fs::path());
        void load(fs::path const& path);
      public:
        loader(fs::path const& folder);
    };
}

#endif

