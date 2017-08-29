#include "platform.hh"
#include "fs.hh"

Platform::platforms Platform::currentOS() {
if (BOOST_OS_WINDOWS != 0) { return windows; }
else if (BOOST_OS_LINUX != 0) { return linux; }
else if (BOOST_OS_MACOS != 0) { return macos; }
else if (BOOST_OS_BSD != 0) { return bsd; }
else if (BOOST_OS_SOLARIS != 0) { return solaris; }
else if (BOOST_OS_UNIX != 0) { return unix; }
}

Platform::Platform() {}

const std::array<const char*,6> Platform::platformNames = {{ "Windows", "Linux", "MacOS", "BSD", "Solaris", "Unix" }}; // Relevant for debug only.