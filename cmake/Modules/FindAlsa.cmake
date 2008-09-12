# - Try to find alsa
# Once done this will define
#
#  ALSA_FOUND - system has ALSA
#  ALSA_INCLUDE_DIRS - the ALSA include directory
#  ALSA_LIBRARIES - Link these to use ALSA
#  ALSA_DEFINITIONS - Compiler switches required for using ALSA
#
#  Copyright (c) 2008 Andreas Schneider <mail@cynapses.org>
#  Modified for other libraries by Lasse Kärkkäinen <tronic>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if (ALSA_LIBRARIES AND ALSA_INCLUDE_DIRS)
  # in cache already
  set(ALSA_FOUND TRUE)
else (ALSA_LIBRARIES AND ALSA_INCLUDE_DIRS)
  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  if (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
    include(UsePkgConfig)
    pkgconfig(alsa _ALSA_INCLUDEDIR _ALSA_LIBDIR _ALSA_LDFLAGS _ALSA_CFLAGS)
  else (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
      pkg_check_modules(_ALSA alsa)
    endif (PKG_CONFIG_FOUND)
  endif (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
  find_path(ALSA_INCLUDE_DIR
    NAMES
      alsa/asoundlib.h
    PATHS
      ${_ALSA_INCLUDEDIR}
      /usr/include
      /usr/local/include
      /opt/local/include
      /sw/include
    SUFFIXES
      ..
  )
  
  find_library(ALSA_LIBRARY
    NAMES
      asound
    PATHS
      ${_ALSA_LIBDIR}
      /usr/lib
      /usr/local/lib
      /opt/local/lib
      /sw/lib
  )

  if (ALSA_LIBRARY)
    set(ALSA_FOUND TRUE)
  endif (ALSA_LIBRARY)

  set(ALSA_INCLUDE_DIRS
    ${ALSA_INCLUDE_DIR}
  )

  if (ALSA_FOUND)
    set(ALSA_LIBRARIES
      ${ALSA_LIBRARIES}
      ${ALSA_LIBRARY}
    )
  endif (ALSA_FOUND)

  if (ALSA_INCLUDE_DIRS AND ALSA_LIBRARIES)
     set(ALSA_FOUND TRUE)
  endif (ALSA_INCLUDE_DIRS AND ALSA_LIBRARIES)

  if (ALSA_FOUND)
    if (NOT ALSA_FIND_QUIETLY)
      message(STATUS "Found ALSA: ${ALSA_LIBRARY}")
    endif (NOT ALSA_FIND_QUIETLY)
  else (ALSA_FOUND)
    if (ALSA_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find ALSA")
    endif (ALSA_FIND_REQUIRED)
  endif (ALSA_FOUND)

  # show the ALSA_INCLUDE_DIRS and ALSA_LIBRARIES variables only in the advanced view
  mark_as_advanced(ALSA_INCLUDE_DIRS ALSA_LIBRARIES)

endif (ALSA_LIBRARIES AND ALSA_INCLUDE_DIRS)

