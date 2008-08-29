# - Try to find the freetype library
# Once done this will define
#
#  FREETYPE_FOUND - system has Freetype
#  FREETYPE_INCLUDE_DIRS - the FREETYPE include directories
#  FREETYPE_LIBRARIES - Link these to use FREETYPE
#  FREETYPE_INCLUDE_DIR is internal and deprecated for use

# Copyright (c) 2006, Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (FREETYPE_LIBRARIES AND FREETYPE_INCLUDE_DIR)

  # in cache already
  set(FREETYPE_FOUND TRUE)

else (FREETYPE_LIBRARIES AND FREETYPE_INCLUDE_DIR)

  FIND_PROGRAM(FREETYPECONFIG_EXECUTABLE NAMES freetype-config PATHS
     /opt/local/bin
  )

  #reset vars
  set(FREETYPE_LIBRARIES)
  set(FREETYPE_INCLUDE_DIR)

  # if freetype-config has been found
  if(FREETYPECONFIG_EXECUTABLE)

    EXEC_PROGRAM(${FREETYPECONFIG_EXECUTABLE} ARGS --libs RETURN_VALUE _return_VALUE OUTPUT_VARIABLE FREETYPE_LIBRARIES)

    EXEC_PROGRAM(${FREETYPECONFIG_EXECUTABLE} ARGS --cflags RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _freetype_pkgconfig_output)
    if(FREETYPE_LIBRARIES AND _freetype_pkgconfig_output)
      set(FREETYPE_FOUND TRUE)

      # freetype-config can print out more than one -I, so we need to chop it up
      # into a list and process each entry separately
      SEPARATE_ARGUMENTS(_freetype_pkgconfig_output)
      FOREACH(value ${_freetype_pkgconfig_output})
        STRING(REGEX REPLACE "-I(.+)" "\\1" value "${value}")
        set(FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR} ${value})
      ENDFOREACH(value)
    endif(FREETYPE_LIBRARIES AND _freetype_pkgconfig_output)

    MARK_AS_ADVANCED(FREETYPE_LIBRARIES FREETYPE_INCLUDE_DIR)

    set( FREETYPE_LIBRARIES ${FREETYPE_LIBRARIES} CACHE INTERNAL "The libraries for freetype" )

  else(FREETYPECONFIG_EXECUTABLE)
	find_path (FREETYPE_INCLUDE_DIR freetype2/freetype/freetype.h)
	set (FREETYPE_INCLUDE_DIR ${FREETYPE_INCLUDE_DIR}/freetype2)
    find_library(FREETYPE_LIBRARIES freetype)
    if(FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARIES)
        set(FREETYPE_FOUND TRUE)
    endif(FREETYPE_INCLUDE_DIR AND FREETYPE_LIBRARIES)
  endif(FREETYPECONFIG_EXECUTABLE)


  IF (FREETYPE_FOUND)
    IF (NOT Freetype_FIND_QUIETLY)
       MESSAGE(STATUS "Found Freetype: ${FREETYPE_LIBRARIES}")
    ENDIF (NOT Freetype_FIND_QUIETLY)
  ELSE (FREETYPE_FOUND)
    IF (Freetype_FIND_REQUIRED)
       MESSAGE(FATAL_ERROR "Could not find FreeType library")
    ENDIF (Freetype_FIND_REQUIRED)
  ENDIF (FREETYPE_FOUND)

endif (FREETYPE_LIBRARIES AND FREETYPE_INCLUDE_DIR)

set(FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR})
