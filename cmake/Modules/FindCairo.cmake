# - Try to find libcairo
# Once done this will define
#
#  CAIRO_FOUND - system has cairo
#  CAIRO_INCLUDE_DIRS - the cairo include directory
#  CAIRO_LIBRARIES - Link these to use cairo
#  CAIRO_DEFINITIONS - Compiler switches required for using cairo
#
#  Copyright (c) 2008 Andreas Schneider <mail@cynapses.org>
#  Modified for other libraries by Lasse Kärkkäinen <tronic>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if (CAIRO_LIBRARIES AND CAIRO_INCLUDE_DIRS)
  # in cache already
  set(CAIRO_FOUND TRUE)
else (CAIRO_LIBRARIES AND CAIRO_INCLUDE_DIRS)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  if (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
    include(UsePkgConfig)
    pkgconfig(cairo _CAIRO_INCLUDEDIR _CAIRO_LIBDIR _CAIRO_LDFLAGS _CAIRO_CFLAGS)
  else (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
      pkg_check_modules(_CAIRO cairo)
    endif (PKG_CONFIG_FOUND)
  endif (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
  find_path(CAIRO_INCLUDE_DIR
    NAMES
      cairo.h
    PATHS
      ${_CAIRO_INCLUDEDIR}
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
    PATH_SUFFIXES
      cairo
  )
  
  find_library(CAIRO_LIBRARY
    NAMES
      cairo
    PATHS
      ${_CAIRO_LIBDIR}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (CAIRO_LIBRARY)
    set(CAIRO_FOUND TRUE)
  endif (CAIRO_LIBRARY)

  set(CAIRO_INCLUDE_DIRS
    ${CAIRO_INCLUDE_DIR}
  )

  if (CAIRO_FOUND)
    set(CAIRO_LIBRARIES
      ${CAIRO_LIBRARIES}
      ${CAIRO_LIBRARY}
    )
  endif (CAIRO_FOUND)

  if (CAIRO_INCLUDE_DIRS AND CAIRO_LIBRARIES)
     set(CAIRO_FOUND TRUE)
  endif (CAIRO_INCLUDE_DIRS AND CAIRO_LIBRARIES)

  if (CAIRO_FOUND)
    if (NOT CAIRO_FIND_QUIETLY)
      message(STATUS "Found cairo: ${CAIRO_LIBRARIES}")
    endif (NOT CAIRO_FIND_QUIETLY)
  else (CAIRO_FOUND)
    if (CAIRO_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find cairo")
    endif (CAIRO_FIND_REQUIRED)
  endif (CAIRO_FOUND)

  # show the CAIRO_INCLUDE_DIRS and CAIRO_LIBRARIES variables only in the advanced view
  mark_as_advanced(CAIRO_INCLUDE_DIRS CAIRO_LIBRARIES)

endif (CAIRO_LIBRARIES AND CAIRO_INCLUDE_DIRS)

