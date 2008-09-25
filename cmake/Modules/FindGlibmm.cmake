# - Try to find glibmm-2.4
# Once done this will define
#
#  GLIBMM_FOUND - system has glibmm
#  GLIBMM_INCLUDE_DIRS - the glibmm include directory
#  GLIBMM_LIBRARIES - Link these to use glibmm
#  GLIBMM_DEFINITIONS - Compiler switches required for using glibmm
#
#  Copyright (c) 2008 Andreas Schneider <mail@cynapses.org>
#  Modified for other libraries by Lasse Kärkkäinen <tronic>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if (GLIBMM_LIBRARIES AND GLIBMM_INCLUDE_DIRS)
  # in cache already
  set(GLIBMM_FOUND TRUE)
else (GLIBMM_LIBRARIES AND GLIBMM_INCLUDE_DIRS)
  # Find the libraries that glibmm depends on
  find_package(Glib)
  if (Glib_FOUND)
    set(${GLIBMM_INCLUDE_DIRS} ${GLIBMM_INCLUDE_DIRS} ${Glib_INCLUDE_DIRS})
    set(${GLIBMM_LIBRARIES} ${GLIBMM_LIBRARIES} ${Glib_LIBRARIES})
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    if (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
      include(UsePkgConfig)
      pkgconfig(glibmm-2.4 _GLIBMM_INCLUDEDIR _GLIBMM_LIBDIR _GLIBMM_LDFLAGS _GLIBMM_CFLAGS)
    else (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
      find_package(PkgConfig)
      if (PKG_CONFIG_FOUND)
        pkg_check_modules(_GLIBMM glibmm-2.4)
      endif (PKG_CONFIG_FOUND)
    endif (${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} EQUAL 4)
    find_path(GLIBMM_INCLUDE_DIR
      NAMES
        glibmm/main.h
      PATHS
        ${_GLIBMM_INCLUDEDIR}
        /usr/include
        /usr/local/include
        /opt/local/include
        /sw/include
      PATH_SUFFIXES
        glibmm-2.4
    )
    
    find_library(GLIBMM_LIBRARY
      NAMES
        glibmm-2.4
      PATHS
        ${_GLIBMM_LIBDIR}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        /sw/lib
    )

    if (GLIBMM_LIBRARY)
      set(GLIBMM_FOUND TRUE)
    endif (GLIBMM_LIBRARY)

    set(GLIBMM_INCLUDE_DIRS
      ${_GLIBMM_INCLUDE_DIRS}
      ${GLIBMM_INCLUDE_DIR}
    )

    if (GLIBMM_FOUND)
      set(GLIBMM_LIBRARIES
        ${GLIBMM_LIBRARIES}
        ${GLIBMM_LIBRARY}
      )
    endif (GLIBMM_FOUND)

  endif (Glib_FOUND)

  if (GLIBMM_INCLUDE_DIRS AND GLIBMM_LIBRARIES)
     set(GLIBMM_FOUND TRUE)
  endif (GLIBMM_INCLUDE_DIRS AND GLIBMM_LIBRARIES)

  if (GLIBMM_FOUND)
    if (NOT GLIBMM_FIND_QUIETLY)
      message(STATUS "Found glibmm: ${GLIBMM_LIBRARIES}")
    endif (NOT GLIBMM_FIND_QUIETLY)
  else (GLIBMM_FOUND)
    if (GLIBMM_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find glibmm or its dependencies")
    endif (GLIBMM_FIND_REQUIRED)
  endif (GLIBMM_FOUND)

  # show the GLIBMM_INCLUDE_DIRS and GLIBMM_LIBRARIES variables only in the advanced view
  mark_as_advanced(GLIBMM_INCLUDE_DIRS GLIBMM_LIBRARIES)

endif (GLIBMM_LIBRARIES AND GLIBMM_INCLUDE_DIRS)

