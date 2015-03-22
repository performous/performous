# - Try to find Atkmm 1.6
# Once done, this will define
#
#  Atkmm_FOUND - system has Atkmm
#  Atkmm_INCLUDE_DIRS - the Atkmm include directories
#  Atkmm_LIBRARIES - link these to use Atkmm

include(LibFindMacros)

# Dependencies
libfind_package(Atkmm Atk)
libfind_package(Atkmm Glibmm)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Atkmm_PKGCONF atkmm-1.6)

# Main include dir
find_path(Atkmm_INCLUDE_DIR
  NAMES atkmm.h
  HINTS ${Atkmm_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES atkmm-1.6
)

# Finally the library itself
find_library(Atkmm_LIBRARY
  NAMES atkmm-1.6
  HINTS ${Atkmm_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Atkmm_PROCESS_INCLUDES Atkmm_INCLUDE_DIR Atk_INCLUDE_DIRS Glibmm_INCLUDE_DIRS)
set(Atkmm_PROCESS_LIBS Atkmm_LIBRARY Atk_LIBRARIES Glibmm_LIBRARIES)
libfind_process(Atkmm)

