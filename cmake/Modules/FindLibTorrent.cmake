# - Try to find libtorrent-rasterbar
# Once done, this will define
#
#  LibTorrent_FOUND - the library is available
#  LibTorrent_INCLUDE_DIRS - the include directories
#  LibTorrent_LIBRARIES - the libraries
#  LibTorrent_INCLUDE - the file to #include (may be used in config.h)
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

# Dependencies
libfind_package(LibTorrent Boost)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(LibTorrent_PKGCONF libtorrent-rasterbar)

# Get definitions from pkg-conf (will b0rk if none found)
set(LibTorrent_DEFINITIONS ${LibTorrent_PKGCONF_CFLAGS_OTHER})

# Main include dir
find_path(LibTorrent_INCLUDE_DIR
  NAMES libtorrent/session.hpp
  PATHS ${LibTorrent_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(LibTorrent_LIBRARY
  NAMES torrent-rasterbar
  PATHS ${LibTorrent_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(LibTorrent_PROCESS_INCLUDES LibTorrent_INCLUDE_DIR Boost_INCLUDE_DIRS)
set(LibTorrent_PROCESS_LIBS LibTorrent_LIBRARY Boost_LIBRARIES)
libfind_process(LibTorrent)

