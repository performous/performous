#pragma once
#ifndef DLL_HPP_INCLUDED
#define DLL_HPP_INCLUDED

#include "dllhelper.hpp"
#include <stdexcept>
#include <string>

namespace plugin {

	/// \brief Exception class for signaling runtime errors from class dll
	class dll_error: public std::runtime_error {
	public:
		dll_error(std::string const& msg): runtime_error(msg) {}
	};

    /// \brief Dynamic library loader
    class DLL_PUBLIC dll {
        void* lib;
    public:
    	/// Open a dynamic library
    	/// \throws dll_error if the library cannot be loaded
        dll(std::string const& filename);
        ~dll();
        /// Get a symbol from DLL (symptr can be a data or a function pointer)
        /// \throws dll_error if no symbol is found
        template <typename SymPtr> void sym(std::string const& name, SymPtr& symptr) {
        	union { void* ptr; SymPtr symptr; } conv;  // Data/function conversion without warnings
        	conv.ptr = sym(name.c_str());
        	if (!conv.ptr) throw dll_error("Symbol " + name + " not found");
        	symptr = conv.symptr;
        }
        /// Get a symbol from DLL (low level C style version)
        void* sym(char const* name);
    };
}

#endif

