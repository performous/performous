#define BUILDING_DLL
#include <plugin++/dll.hpp>
#undef BUILDING_DLL

#include <windows.h>
#include <stdexcept>

namespace plugin {
	dll::dll(std::string const& filename): lib(LoadLibrary(filename.c_str())) {
		if (!lib) throw std::runtime_error("Unable to open " + filename);
	}

	dll::~dll() {
		FreeLibrary(static_cast<HINSTANCE>(lib));
	}
}

