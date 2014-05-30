# - Try to find SigC++-2.0
# Once done, this will define
#
#  SigC++_FOUND - system has SigC++
#  SigC++_INCLUDE_DIRS - the SigC++ include directories
#  SigC++_LIBRARIES - link these to use SigC++

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(SigC++_PKGCONF sigc++-2.0)

# Main include dir
find_path(SigC++_INCLUDE_DIR
  NAMES sigc++/sigc++.h
  HINTS ${SigC++_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES sigc++-2.0
)

# Glib-related libraries also use a separate config header, which is in lib dir
find_path(SigC++Config_INCLUDE_DIR
  NAMES sigc++config.h
  HINTS ${SigC++_PKGCONF_INCLUDE_DIRS} /usr
  PATH_SUFFIXES lib/sigc++-2.0/include ../lib/sigc++-2.0/include
)

# Finally the library itself
find_library(SigC++_LIBRARY
  NAMES sigc-2.0
  HINTS ${SigC++_PKGCONF_LIBRARY_DIRS}
)

set(SigC++_PROCESS_INCLUDES SigC++_INCLUDE_DIR SigC++Config_INCLUDE_DIR)
set(SigC++_PROCESS_LIBS SigC++_LIBRARY)
libfind_process(SigC++)

