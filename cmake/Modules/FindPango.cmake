# - Try to find Pango
# Once done, this will define
#
#  Pango_FOUND - system has Pango
#  Pango_INCLUDE_DIRS - the Pango include directories
#  Pango_LIBRARIES - link these to use Pango

include(LibFindMacros)

# Dependencies
libfind_package(Pango Freetype)
libfind_package(Pango Glib)
libfind_package(Pango GObject)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Pango_PKGCONF pango)

# Include dir
find_path(Pango_INCLUDE_DIR
  NAMES pango/pango.h
  HINTS ${Pango_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES pango-1.0
)

# Finally the library itself
find_library(Pango_LIBRARY
  NAMES pango-1.0
  HINTS ${Pango_PKGCONF_LIBRARY_DIRS}
)

libfind_process(Pango)

