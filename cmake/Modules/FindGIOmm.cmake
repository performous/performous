# - Try to find GIOmm 2.4
# Once done, this will define
#
#  GIOmm_FOUND - system has GIOmm
#  GIOmm_INCLUDE_DIRS - the GIOmm include directories
#  GIOmm_LIBRARIES - link these to use GIOmm

include(LibFindMacros)

# Dependencies
libfind_package(GIOmm GIO)
libfind_package(GIOmm Glibmm)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(GIOmm_PKGCONF giomm-2.4)

# Main include dir
find_path(GIOmm_INCLUDE_DIR
  NAMES giomm.h
  HINTS ${GIOmm_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES giomm-2.4
)

# Finally the library itself
find_library(GIOmm_LIBRARY
  NAMES giomm-2.4
  HINTS ${GIOmm_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(GIOmm_PROCESS_INCLUDES GIOmm_INCLUDE_DIR GIO_INCLUDE_DIRS Glibmm_INCLUDE_DIRS)
set(GIOmm_PROCESS_LIBS GIOmm_LIBRARY GIO_LIBRARIES Glibmm_LIBRARIES)
libfind_process(GIOmm)

