#ifndef DLL_HPP_INCLUDED
#define DLL_HPP_INCLUDED

#include "dllhelper.hpp"
#include <string>

namespace plugin {
    /** @short RAII wrapper to load and unload dynamic library **/
    class DLL_PUBLIC dll {
        void* lib;
      public:
        dll(std::string const& filename);
        ~dll();
    };
}

#endif

