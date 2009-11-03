#pragma once
#include <string>

namespace plugin {
	/// Get the current executable name with path. Returns empty path if the name
	/// cannot be found. May return absolute or relative paths.
	std::string execname();
}

