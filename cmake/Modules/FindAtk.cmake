# - Try to find Atk 1.0
# Once done, this will define
#
#  Atk_FOUND - system has Atk
#  Atk_INCLUDE_DIRS - the Atk include directories
#  Atk_LIBRARIES - link these to use Atk

include(LibFindMacros)

# Dependencies
libfind_package(Atk GObject)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Atk_PKGCONF atk)

# Main include dir
find_path(Atk_INCLUDE_DIR
  NAMES atk/atk.h
  PATHS ${Atk_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES atk-1.0
)

# Find the library
find_library(Atk_LIBRARY
  NAMES atk-1.0
  PATHS ${Atk_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Atk_PROCESS_INCLUDES Atk_INCLUDE_DIR)
set(Atk_PROCESS_LIBS Atk_LIBRARY GObject_LIBRARIES)
libfind_process(Atk)

