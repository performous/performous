# - Try to find Glibmm-2.68 or Glibmm-2.4 (defaulting)
# Once done, this will define
#
#  Glibmm_FOUND - system has Glibmm
#  Glibmm_INCLUDE_DIRS - the Glibmm include directories
#  Glibmm_LIBRARIES - link these to use Glibmm

include(LibFindMacros)

# Dependencies
libfind_package(Glibmm Glib)
libfind_package(Glibmm SigC++)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  # Use pkg-config to get hints about paths
  libfind_pkg_check_modules(Glibmm_PKGCONF_2_68 glibmm-2.68)
  if(Glibmm_PKGCONF_2_68_FOUND)
    set(Glibmm_VERSION "2.68")
    set(Glibmm_VERSION_2_4 "0")
    set(Glibmm_VERSION_2_68 "1")
    set(Glibmm_PKGCONF_INCLUDE_DIRS ${Glibmm_PKGCONF_2_68_INCLUDE_DIRS})
    set(Glibmm_PKGCONF_LIBRARY_DIRS ${Glibmm_PKGCONF_2_68_LIBRARY_DIRS})
  else()
    libfind_pkg_check_modules(Glibmm_PKGCONF_2_4 glibmm-2.4)
    if(Glibmm_PKGCONF_2_4_FOUND)
      set(Glibmm_VERSION "2.4")
      set(Glibmm_VERSION_2_4 "1")
      set(Glibmm_VERSION_2_68 "0")
      set(Glibmm_PKGCONF_INCLUDE_DIRS ${Glibmm_PKGCONF_2_4_INCLUDE_DIRS})
      set(Glibmm_PKGCONF_LIBRARY_DIRS ${Glibmm_PKGCONF_2_4_LIBRARY_DIRS})
    endif()
  endif()
else()
  set(Glibmm_VERSION "2.4")
  set(Glibmm_VERSION_2_4 "1")
  set(Glibmm_VERSION_2_68 "0")
  set(Glibmm_PKGCONF_INCLUDE_DIRS ${Glibmm_PKGCONF_2_4_INCLUDE_DIRS})
  set(Glibmm_PKGCONF_LIBRARY_DIRS ${Glibmm_PKGCONF_2_4_LIBRARY_DIRS})
endif()

# Main include dir
find_path(Glibmm_INCLUDE_DIR
  NAMES glibmm/main.h
  HINTS ${Glibmm_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES glibmm-${Glibmm_VERSION}
)

# Glib-related libraries also use a separate config header, which is in lib dir
find_path(GlibmmConfig_INCLUDE_DIR
  NAMES glibmmconfig.h
  HINTS ${Glibmm_PKGCONF_INCLUDE_DIRS} /usr
  PATH_SUFFIXES lib/glibmm-${Glibmm_VERSION}/include ../lib/glibmm-${Glibmm_VERSION}/include
)

# Finally the library itself
find_library(Glibmm_LIBRARY
  NAMES glibmm-${Glibmm_VERSION}
  HINTS ${Glibmm_PKGCONF_LIBRARY_DIRS}
)

set(Glibmm_PROCESS_INCLUDES GlibmmConfig_INCLUDE_DIR)
libfind_process(Glibmm)

