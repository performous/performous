# - Try to find LibXML++ 5.0, 3.0 or 2.6 (defaulting)
# Once done, this will define
#
#  LibXML++_FOUND - system has LibXML++
#  LibXML++_INCLUDE_DIRS - the LibXML++ include directories
#  LibXML++_LIBRARIES - link these to use LibXML++

include(LibFindMacros)

# Dependencies
libfind_package(LibXML++ LibXML2)
libfind_package(LibXML++ Glibmm)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  # Use pkg-config to get hints about paths
  libfind_pkg_check_modules(LibXML++_PKGCONF_5_0 libxml++-5.0)
  if(LibXML++_PKGCONF_5_0_FOUND)
    set(LibXML++_VERSION "5.0")
    set(LibXML++_VERSION_2_6 "0")
    set(LibXML++_VERSION_3_0 "0")
    set(LibXML++_VERSION_5_0 "1")
    set(LibXML++_PKGCONF_INCLUDE_DIRS ${LibXML++_PKGCONF_5_0_INCLUDE_DIRS})
    set(LibXML++_PKGCONF_LIBRARY_DIRS ${LibXML++_PKGCONF_5_0_LIBRARY_DIRS})
  else()
    libfind_pkg_check_modules(LibXML++_PKGCONF_3_0 libxml++-3.0)
    if(LibXML++_PKGCONF_3_0_FOUND)
      set(LibXML++_VERSION "3.0")
      set(LibXML++_VERSION_2_6 "0")
      set(LibXML++_VERSION_3_0 "1")
      set(LibXML++_VERSION_5_0 "0")
      set(LibXML++_PKGCONF_INCLUDE_DIRS ${LibXML++_PKGCONF_3_0_INCLUDE_DIRS})
      set(LibXML++_PKGCONF_LIBRARY_DIRS ${LibXML++_PKGCONF_3_0_LIBRARY_DIRS})
    else()  
      libfind_pkg_check_modules(LibXML++_PKGCONF_2_6 libxml++-2.6)
      if(LibXML++_PKGCONF_2_6_FOUND)
        set(LibXML++_VERSION "2.6")
        set(LibXML++_VERSION_2_6 "1")
        set(LibXML++_VERSION_3_0 "0")
        set(LibXML++_VERSION_5_0 "0")
        set(LibXML++_PKGCONF_INCLUDE_DIRS ${LibXML++_PKGCONF_2_6_INCLUDE_DIRS})
        set(LibXML++_PKGCONF_LIBRARY_DIRS ${LibXML++_PKGCONF_2_6_LIBRARY_DIRS})
      endif()
    endif()
  endif()
else()
  set(LibXML++_VERSION "2.6")
  set(LibXML++_VERSION_2_6 "1")
  set(LibXML++_VERSION_3_0 "0")
  set(LibXML++_VERSION_5_0 "0")
  set(LibXML++_PKGCONF_INCLUDE_DIRS ${LibXML++_PKGCONF_2_6_INCLUDE_DIRS})
  set(LibXML++_PKGCONF_LIBRARY_DIRS ${LibXML++_PKGCONF_2_6_LIBRARY_DIRS})
endif()

# Main include dir
find_path(LibXML++_INCLUDE_DIR
  NAMES libxml++/libxml++.h
  HINTS ${LibXML++_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES libxml++-${LibXML++_VERSION}
)

# Glib-related libraries also use a separate config header, which is in lib dir
find_path(LibXML++Config_INCLUDE_DIR
  NAMES libxml++config.h
  HINTS ${LibXML++_PKGCONF_INCLUDE_DIRS} /usr /usr/lib/x86_64-linux-gnu/libxml++-${LibXML++_VERSION}/include/
  PATH_SUFFIXES lib/libxml++-${LibXML++_VERSION}/include ../lib/libxml++-${LibXML++_VERSION}/include
)

# Finally the library itself
find_library(LibXML++_LIBRARY
  NAMES xml++-${LibXML++_VERSION}
  HINTS ${LibXML++_PKGCONF_LIBRARY_DIRS}
)

set(LibXML++_PROCESS_INCLUDES LibXML++Config_INCLUDE_DIR)
libfind_process(LibXML++)

