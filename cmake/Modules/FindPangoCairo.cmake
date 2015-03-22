# - Try to find PangoCairo
# Once done, this will define
#
#  PangoCairo_FOUND - system has Pango
#  PangoCairo_INCLUDE_DIRS - the Pango include directories
#  PangoCairo_LIBRARIES - link these to use Pango

include(LibFindMacros)

# Dependencies
libfind_package(PangoCairo Pango)
libfind_package(PangoCairo Cairo)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(PangoCairo_PKGCONF pangocairo)

# Include dir
find_path(PangoCairo_INCLUDE_DIR
  NAMES pango/pangocairo.h
  HINTS ${PangoCairo_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES pango-1.0
)

# Finally the library itself
find_library(PangoCairo_LIBRARY
  NAMES pangocairo-1.0
  HINTS ${PangoCairo_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(PangoCairo_PROCESS_INCLUDES PangoCairo_INCLUDE_DIR Pango_INCLUDE_DIR Cairo_INCLUDE_DIR)
set(PangoCairo_PROCESS_LIBS PangoCairo_LIBRARY Pango_LIBRARY Cairo_LIBRARY)
libfind_process(PangoCairo)

