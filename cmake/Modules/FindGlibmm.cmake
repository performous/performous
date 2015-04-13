# - Try to find Glibmm-2.4
# Once done, this will define
#
#  Glibmm_FOUND - system has Glibmm
#  Glibmm_INCLUDE_DIRS - the Glibmm include directories
#  Glibmm_LIBRARIES - link these to use Glibmm

include(LibFindMacros)

# Dependencies
libfind_package(Glibmm Glib)
libfind_package(Glibmm SigC++)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Glibmm_PKGCONF glibmm-2.4)

# Main include dir
find_path(Glibmm_INCLUDE_DIR
  NAMES glibmm/main.h
  HINTS ${Glibmm_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES glibmm-2.4
)

# Glib-related libraries also use a separate config header, which is in lib dir
find_path(GlibmmConfig_INCLUDE_DIR
  NAMES glibmmconfig.h
  HINTS ${Glibmm_PKGCONF_INCLUDE_DIRS} /usr
  PATH_SUFFIXES lib/glibmm-2.4/include ../lib/glibmm-2.4/include
)

# Finally the library itself
find_library(Glibmm_LIBRARY
  NAMES glibmm-2.4
  HINTS ${Glibmm_PKGCONF_LIBRARY_DIRS}
)

set(Glibmm_PROCESS_INCLUDES GlibmmConfig_INCLUDE_DIR)
libfind_process(Glibmm)

