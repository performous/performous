#pragma once
#include <boost/filesystem.hpp>

/// Get the current executable name with path. Returns empty path if the name
/// cannot be found. May return absolute or relative paths.
boost::filesystem::path selfpath();

