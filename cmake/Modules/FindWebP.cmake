# - Try to find Google's WebP Library
# Once done, this will define
#
#  WebP_FOUND - system has WebP
#  WebP_INCLUDE_DIRS - the WebP include directories
#  WebP_LIBRARIES - link these to use WebP

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(WebP_PKGCONF freetype2)

# Include dir
find_path(WebP_INCLUDE_DIR
  NAMES webp/decode.h webp/types.h
  HINTS ${WebP_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES webp
)

# Finally the library itself
find_library(WebP_LIBRARY
  NAMES webp
  HINTS ${WebP_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(WebP_PROCESS_INCLUDES WebP_INCLUDE_DIR)
set(WebP_PROCESS_LIBS WebP_LIBRARY)
libfind_process(WebP)

