# - Try to find LibRSVG
# Once done, this will define
#
#  LibRSVG_FOUND - system has LibRSVG
#  LibRSVG_INCLUDE_DIRS - the LibRSVG include directories
#  LibRSVG_LIBRARIES - link these to use LibRSVG

include(LibFindMacros)

# Dependencies
libfind_package(LibRSVG Cairo)
libfind_package(LibRSVG GDK-PixBuf)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(LibRSVG_PKGCONF librsvg-2.0)

# Include dir
find_path(LibRSVG_INCLUDE_DIR
  NAMES librsvg/rsvg.h
  PATHS ${LibRSVG_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES librsvg-2
)

# Finally the library itself
find_library(LibRSVG_LIBRARY
  NAMES rsvg-2
  PATHS ${LibRSVG_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(LibRSVG_PROCESS_INCLUDES LibRSVG_INCLUDE_DIR Cairo_INCLUDE_DIRS GDK-PixBuf_INCLUDE_DIRS)
set(LibRSVG_PROCESS_LIBS LibRSVG_LIBRARY Cairo_LIBRARIES GDK-PixBuf_LIBRARIES)
libfind_process(LibRSVG)

