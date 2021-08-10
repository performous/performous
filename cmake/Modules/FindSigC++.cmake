# - Try to find SigC++3.0 or SigC++-2.0 (defaulting)
# Once done, this will define
#
#  SigC++_FOUND - system has SigC++
#  SigC++_INCLUDE_DIRS - the SigC++ include directories
#  SigC++_LIBRARIES - link these to use SigC++

include(LibFindMacros)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  # Use pkg-config to get hints about paths
  libfind_pkg_check_modules(SigC++_PKGCONF_3_0 sigc++-3.0)
  if(SigC++_PKGCONF_3_0_FOUND)
    set(SigC++_VERSION "3.0")
    set(SigC++_VERSION_2_0 "0")
    set(SigC++_VERSION_3_0 "1")
    set(SigC++_PKGCONF_INCLUDE_DIRS ${SigC++_PKGCONF_3_0_INCLUDE_DIRS})
    set(SigC++_PKGCONF_LIBRARY_DIRS ${SigC++_PKGCONF_3_0_LIBRARY_DIRS})
  else()
    libfind_pkg_check_modules(SigC++_PKGCONF_2_0 sigc++-2.0)
    if(SigC++_PKGCONF_2_0_FOUND)
      set(SigC++_VERSION "2.0")
      set(SigC++_VERSION_2_0 "1")
      set(SigC++_VERSION_3_0 "0")
      set(SigC++_PKGCONF_INCLUDE_DIRS ${SigC++_PKGCONF_2_0_INCLUDE_DIRS})
      set(SigC++_PKGCONF_LIBRARY_DIRS ${SigC++_PKGCONF_2_0_LIBRARY_DIRS})
    endif()
  endif()
else()
  set(SigC++_VERSION "2.0")
  set(SigC++_VERSION_2_0 "1")
  set(SigC++_VERSION_3_0 "0")
  set(SigC++_PKGCONF_INCLUDE_DIRS ${SigC++_PKGCONF_2_0_INCLUDE_DIRS})
  set(SigC++_PKGCONF_LIBRARY_DIRS ${SigC++_PKGCONF_2_0_LIBRARY_DIRS})
endif()

# Main include dir
find_path(SigC++_INCLUDE_DIR
  NAMES sigc++/sigc++.h
  HINTS ${SigC++_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES sigc++-${SigC++_VERSION}
)

# Glib-related libraries also use a separate config header, which is in lib dir
find_path(SigC++Config_INCLUDE_DIR
  NAMES sigc++config.h
  HINTS ${SigC++_PKGCONF_INCLUDE_DIRS} /usr
  PATH_SUFFIXES lib/sigc++-${SigC++_VERSION}/include ../lib/sigc++-${SigC++_VERSION}/include
)

# Finally the library itself
find_library(SigC++_LIBRARY
  NAMES sigc-${SigC++_VERSION}
  HINTS ${SigC++_PKGCONF_LIBRARY_DIRS}
)

set(SigC++_PROCESS_INCLUDES SigC++_INCLUDE_DIR SigC++Config_INCLUDE_DIR)
set(SigC++_PROCESS_LIBS SigC++_LIBRARY)
libfind_process(SigC++)