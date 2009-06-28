#include <plugin++/dll.hpp>

#include <windows.h>
#include <stdexcept>

namespace plugin {
	dll::dll(fs::path const& filename): lib(LoadLibrary(filename.string().c_str())) {
		if (!lib) throw std::runtime_error(dlerror());
	}

	dll::~dll() {
		FreeLibrary(static_cast<HINSTANCE*>(lib));
	}
}

