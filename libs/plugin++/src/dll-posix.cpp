#include <plugin++/dll.hpp>

#include <dlfcn.h>
#include <stdexcept>

namespace plugin {
	dll::dll(std::string const& filename): lib(dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL)) {
		if (!lib) throw std::runtime_error(dlerror());
	}

	dll::~dll() {
		dlclose(lib);
	}
}

