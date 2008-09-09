# - Try to find libxml2
# Once done this will define
#
#  LIBXML2_FOUND - system has xml2
#  LIBXML2_INCLUDE_DIRS - the xml2 include directory
#  LIBXML2_LIBRARIES - Link these to use xml2
#  LIBXML2_DEFINITIONS - Compiler switches required for using xml2
#
#  Copyright (c) 2008 Andreas Schneider <mail@cynapses.org>
#  Modified for other libraries by Lasse Kärkkäinen <tronic>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (LIBXML2_LIBRARIES AND LIBXML2_INCLUDE_DIRS)
  # in cache already
  set(LIBXML2_FOUND TRUE)
else (LIBXML2_LIBRARIES AND LIBXML2_INCLUDE_DIRS)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  if (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
    include(UsePkgConfig)
    pkgconfig(libxml-2.0 _LIBXML2_INCLUDEDIR _LIBXML2_LIBDIR _LIBXML2_LDFLAGS _LIBXML2_CFLAGS)
  else (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
      pkg_check_modules(_LIBXML2 libxml-2.0)
    endif (PKG_CONFIG_FOUND)
  endif (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
  find_path(LIBXML2_INCLUDE_DIR
    NAMES
      libxml/xpath.h
    PATHS
      ${_LIBXML2_INCLUDEDIR}
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
    PATH_SUFFIXES
      libxml2
  )
  
  find_library(LIBXML2_LIBRARY
    NAMES
      xml2
    PATHS
      ${_LIBXML2_LIBDIR}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (LIBXML2_LIBRARY)
    set(LIBXML2_FOUND TRUE)
  endif (LIBXML2_LIBRARY)

  set(LIBXML2_INCLUDE_DIRS
    ${LIBXML2_INCLUDE_DIR}
  )

  if (LIBXML2_FOUND)
    set(LIBXML2_LIBRARIES
      ${LIBXML2_LIBRARIES}
      ${LIBXML2_LIBRARY}
    )
  endif (LIBXML2_FOUND)

  if (LIBXML2_INCLUDE_DIRS AND LIBXML2_LIBRARIES)
     set(LIBXML2_FOUND TRUE)
  endif (LIBXML2_INCLUDE_DIRS AND LIBXML2_LIBRARIES)

  if (LIBXML2_FOUND)
    if (NOT LIBXML2_FIND_QUIETLY)
      message(STATUS "Found libxml2: ${LIBXML2_LIBRARY}")
    endif (NOT LIBXML2_FIND_QUIETLY)
  else (LIBXML2_FOUND)
    if (LIBXML2_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libxml2")
    endif (LIBXML2_FIND_REQUIRED)
  endif (LIBXML2_FOUND)

  # show the LIBXML2_INCLUDE_DIRS and LIBXML2_LIBRARIES variables only in the advanced view
  mark_as_advanced(LIBXML2_INCLUDE_DIRS LIBXML2_LIBRARIES)

endif (LIBXML2_LIBRARIES AND LIBXML2_INCLUDE_DIRS)

