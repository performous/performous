# - Try to find GTKmm 2.4
# Once done, this will define
#
#  GTKmm_FOUND - system has GTKmm
#  GTKmm_INCLUDE_DIRS - the GTKmm include directories
#  GTKmm_LIBRARIES - link these to use GTKmm

include(LibFindMacros)

# Dependencies
libfind_package(GTKmm GTK)
libfind_package(GTKmm Glibmm)
libfind_package(GTKmm GIOmm)
libfind_package(GTKmm GDKmm)
libfind_package(GTKmm Pangomm)
libfind_package(GTKmm Atkmm)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(GTKmm_PKGCONF gtkmm-2.4)

# Main include dir
find_path(GTKmm_INCLUDE_DIR
  NAMES gtkmm.h
  HINTS ${GTKmm_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES gtkmm-2.4
)

# Glib-related libraries also use a separate config header, which is in lib dir
find_path(GTKmmConfig_INCLUDE_DIR
  NAMES gtkmmconfig.h
  HINTS ${GTKmm_PKGCONF_INCLUDE_DIRS} /usr
  PATH_SUFFIXES lib/gtkmm-2.4/include
)

# Finally the library itself
find_library(GTKmm_LIBRARY
  NAMES gtkmm-2.4
  HINTS ${GTKmm_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(GTKmm_PROCESS_INCLUDES GTKmm_INCLUDE_DIR GTKmmConfig_INCLUDE_DIR GTK_INCLUDE_DIRS Glibmm_INCLUDE_DIRS GIOmm_INCLUDE_DIRS GDKmm_INCLUDE_DIRS Pangomm_INCLUDE_DIRS Atkmm_INCLUDE_DIRS)
set(GTKmm_PROCESS_LIBS GTKmm_LIBRARY GTK_LIBRARIES Glibmm_LIBRARIES GIOmm_LIBRARIES GDKmm_LIBRARIES Pangomm_LIBRARIES Atkmm_LIBRARIES)
libfind_process(GTKmm)

