# - Try to find GObject 2.0
# Once done, this will define
#
#  GObject_FOUND - system has GObject
#  GObject_LIBRARIES - link these to use GObject

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(GObject_PKGCONF gobject-2.0)

# Find the library
find_library(GObject_LIBRARY
  NAMES gobject-2.0
  PATHS ${GObject_PKGCONF_LIBRARY_DIRS}
)

set(GObject_PROCESS_LIBS GObject_LIBRARY Glib_LIBRARIES)
libfind_process(GObject)

