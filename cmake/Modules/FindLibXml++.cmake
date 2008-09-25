# - Try to find libxml++-2.6
# Once done this will define
#
#  LIBXMLPP_FOUND - system has libxml++
#  LIBXMLPP_INCLUDE_DIRS - the libxml++ include directory
#  LIBXMLPP_LIBRARIES - Link these to use libxml++
#  LIBXMLPP_DEFINITIONS - Compiler switches required for using libxml++
#
#  Copyright (c) 2008 Andreas Schneider <mail@cynapses.org>
#  Modified for other libraries by Lasse Kärkkäinen <tronic>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if (LIBXMLPP_LIBRARIES AND LIBXMLPP_INCLUDE_DIRS)
  # in cache already
  set(LIBXMLPP_FOUND TRUE)
else (LIBXMLPP_LIBRARIES AND LIBXMLPP_INCLUDE_DIRS)
  # Find the libraries that libxml++ depends on
  find_package(LibXml2)
  find_package(Glibmm)
  if (LIBXML2_FOUND AND GLIBMM_FOUND)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    if (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
      include(UsePkgConfig)
      pkgconfig(libxml++-2.6 _LIBXMLPP_INCLUDEDIR _LIBXMLPP_LIBDIR _LIBXMLPP_LDFLAGS _LIBXMLPP_CFLAGS)
    else (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
      find_package(PkgConfig)
      if (PKG_CONFIG_FOUND)
        pkg_check_modules(_LIBXMLPP libxml++-2.6)
      endif (PKG_CONFIG_FOUND)
    endif (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
    find_path(LIBXMLPP_INCLUDE_DIR
      NAMES
        libxml++/libxml++.h
      PATHS
        ${_LIBXMLPP_INCLUDEDIR}
        /usr/include
        /usr/local/include
        /opt/local/include
        /sw/include
      PATH_SUFFIXES
        libxml++-2.6
    )
    
    find_library(LIBXMLPP_LIBRARY
      NAMES
        xml++-2.6
      PATHS
        ${_LIBXMLPP_LIBDIR}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
    )

    if (LIBXMLPP_LIBRARY)
      set(LIBXMLPP_FOUND TRUE)
    endif (LIBXMLPP_LIBRARY)

    set(LIBXMLPP_INCLUDE_DIRS
      ${_LIBXMLPP_INCLUDE_DIRS}
      ${LIBXMLPP_INCLUDE_DIR}
      ${LIBXML2_INCLUDE_DIRS}
      ${GLIBMM_INCLUDE_DIRS}
    )

    if (LIBXMLPP_FOUND)
      set(LIBXMLPP_LIBRARIES
        ${LIBXMLPP_LIBRARIES}
        ${LIBXMLPP_LIBRARY}
        ${LIBXML2_LIBRARIES}
        ${GLIBMM_LIBRARIES}
      )
    endif (LIBXMLPP_FOUND)

  endif (LIBXML2_FOUND AND GLIBMM_FOUND)

  if (LIBXMLPP_INCLUDE_DIRS AND LIBXMLPP_LIBRARIES)
     set(LIBXMLPP_FOUND TRUE)
  endif (LIBXMLPP_INCLUDE_DIRS AND LIBXMLPP_LIBRARIES)

  if (LIBXMLPP_FOUND)
    if (NOT LIBXMLPP_FIND_QUIETLY)
      message(STATUS "Found libxml++: ${LIBXMLPP_LIBRARY}")
    endif (NOT LIBXMLPP_FIND_QUIETLY)
  else (LIBXMLPP_FOUND)
    if (LIBXMLPP_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libxml++ or its dependencies")
    endif (LIBXMLPP_FIND_REQUIRED)
  endif (LIBXMLPP_FOUND)

  # show the LIBXMLPP_INCLUDE_DIRS and LIBXMLPP_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBXMLPP_INCLUDE_DIRS LIBXMLPP_LIBRARIES)

endif (LIBXMLPP_LIBRARIES AND LIBXMLPP_INCLUDE_DIRS)

