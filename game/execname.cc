#include "platform.hh"
#include "fs.hh"

/// Get the current executable name with path. Returns empty path if the name cannot be found. May return absolute or relative paths.

#if (BOOST_OS_WINDOWS)
#include "platform/execname.win.inc"
#elif (BOOST_OS_MACOS)
#include "platform/execname.mac.inc"
#elif (BOOST_OS_BSD)
#include "platform/execname.bsd.inc"
#elif (BOOST_OS_SOLARIS)
#include "platform/execname.sun.inc"
#elif (BOOST_OS_LINUX)
#include "platform/execname.unix.inc"
#else
	return fs::path();
#endif // BOOST_OS_WINDOWS