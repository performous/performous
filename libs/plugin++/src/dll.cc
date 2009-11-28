#define BUILDING_DLL
#include <plugin++/dll.hpp>
#undef BUILDING_DLL

#include <stdexcept>

using namespace plugin;

#ifdef __WIN32

#include <windows.h>

dll::dll(std::string const& filename): lib(LoadLibrary(filename.c_str())) {
	if (!lib) throw dll_error("Unable to open " + filename);
}

dll::~dll() { FreeLibrary(static_cast<HINSTANCE>(lib)); }

void* dll::sym(char const* sym) {
	union{
		void* ptr;
		FARPROC fptr;
	}conv;
	conv.fptr = GetProcAddress(static_cast<HINSTANCE>(lib), sym);
	return conv.ptr;
}

#else

#include <dlfcn.h>

dll::dll(std::string const& filename): lib(dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL)) {
	if (!lib) throw dll_error(dlerror());
}

dll::~dll() { dlclose(lib); }

void* dll::sym(char const* sym) {
	return dlsym(lib, sym);
}

#endif
