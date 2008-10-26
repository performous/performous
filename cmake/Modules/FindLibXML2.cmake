# - Try to find libxml2
# Once done this will define
#
#  LibXML2_FOUND - system has xml2
#  LibXML2_INCLUDE_DIRS - the xml2 include directory
#  LibXML2_LIBRARIES - Link these to use xml2
#  LibXML2_DEFINITIONS - Compiler switches required for using xml2
#
#  Copyright (c) 2008 Andreas Schneider <mail@cynapses.org>
#  Modified for other libraries by Lasse Kärkkäinen <tronic>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (LibXML2_LIBRARIES AND LibXML2_INCLUDE_DIRS)
  # in cache already
  set(LibXML2_FOUND TRUE)
else (LibXML2_LIBRARIES AND LibXML2_INCLUDE_DIRS)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  if (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
    include(UsePkgConfig)
    pkgconfig(libxml-2.0 _LibXML2_INCLUDEDIR _LibXML2_LIBDIR _LibXML2_LDFLAGS _LibXML2_CFLAGS)
  else (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
      pkg_check_modules(_LIBXML2 libxml-2.0)
    endif (PKG_CONFIG_FOUND)
  endif (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
  find_path(LibXML2_INCLUDE_DIR
    NAMES
      libxml/xpath.h
    PATHS
      ${_LibXML2_INCLUDEDIR}
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
    PATH_SUFFIXES
      libxml2
  )
  
  find_library(LibXML2_LIBRARY
    NAMES
      xml2
    PATHS
      ${_LibXML2_LIBDIR}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (LibXML2_LIBRARY)
    set(LibXML2_FOUND TRUE)
  endif (LibXML2_LIBRARY)

  set(LibXML2_INCLUDE_DIRS
    ${LibXML2_INCLUDE_DIR}
  )

  if (LibXML2_FOUND)
    set(LibXML2_LIBRARIES
      ${LibXML2_LIBRARIES}
      ${LibXML2_LIBRARY}
    )
  endif (LibXML2_FOUND)

  if (LibXML2_INCLUDE_DIRS AND LibXML2_LIBRARIES)
     set(LibXML2_FOUND TRUE)
  endif (LibXML2_INCLUDE_DIRS AND LibXML2_LIBRARIES)

  if (LibXML2_FOUND)
    if (NOT LibXML2_FIND_QUIETLY)
      message(STATUS "Found libxml2: ${LibXML2_LIBRARY}")
    endif (NOT LibXML2_FIND_QUIETLY)
  else (LibXML2_FOUND)
    if (LibXML2_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find libxml2")
    endif (LibXML2_FIND_REQUIRED)
  endif (LibXML2_FOUND)

  # show the LibXML2_INCLUDE_DIRS and LibXML2_LIBRARIES variables only in the advanced view
  mark_as_advanced(LibXML2_INCLUDE_DIRS LibXML2_LIBRARIES)

endif (LibXML2_LIBRARIES AND LibXML2_INCLUDE_DIRS)

