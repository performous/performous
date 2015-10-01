# -*- cmake -*-

# - Find JSONCpp
# Find the JSONCpp includes and library
# This module defines
#  JSONCPP_INCLUDE_DIRS, where to find json.h, etc.
#  JSONCPP_LIBRARIES, the libraries needed to use jsoncpp.
#  JSONCPP_FOUND, If false, do not try to use jsoncpp.
#  also defined, but not for general use are
#  JSONCPP_LIBRARY, where to find the jsoncpp library.

FIND_PATH(JSONCPP_INCLUDE_DIRS json/json.h json/autolink.h json/features.h json/value.h json/config.h json/forwards.h json/reader.h json/writer.h
/usr/local/include
/usr/include
/usr/include/jsoncpp
/usr/include/jsoncpp/json
)

# Get the GCC compiler version
EXEC_PROGRAM(${CMAKE_CXX_COMPILER}
			ARGS ${CMAKE_CXX_COMPILER_ARG1} -dumpversion
			OUTPUT_VARIABLE _gcc_COMPILER_VERSION
			OUTPUT_STRIP_TRAILING_WHITESPACE
			)

SET(JSONCPP_NAMES ${JSONCPP_NAMES} libjson_linux-gcc-${_gcc_COMPILER_VERSION}_libmt.so)
FIND_LIBRARY(JSONCPP_LIBRARY
  NAMES ${JSONCPP_NAMES} libjsoncpp.so libjsoncpp.a
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
  )

IF (JSONCPP_LIBRARY AND JSONCPP_INCLUDE_DIRS)
	SET(JSONCPP_LIBRARIES ${JSONCPP_LIBRARY})
	SET(JSONCPP_FOUND "YES")
ELSE (JSONCPP_LIBRARY AND JSONCPP_INCLUDE_DIRS)
  SET(JSONCPP_FOUND "NO")
ENDIF (JSONCPP_LIBRARY AND JSONCPP_INCLUDE_DIRS)


IF (JSONCPP_FOUND)
   IF (NOT JSONCPP_FIND_QUIETLY)
	  MESSAGE(STATUS "Found JSONCpp: ${JSONCPP_LIBRARIES}")
	  MESSAGE(STATUS "JsonCPP include dirs: " ${JSONCPP_INCLUDE_DIRS})
   ENDIF (NOT JSONCPP_FIND_QUIETLY)
ELSE (JSONCPP_FOUND)
   IF (JSONCPP_FIND_REQUIRED)
	  MESSAGE(FATAL_ERROR "Could not find JSONCpp library")
   ENDIF (JSONCPP_FIND_REQUIRED)
ENDIF (JSONCPP_FOUND)



MARK_AS_ADVANCED(
  JSONCPP_LIBRARY
  JSONCPP_INCLUDE_DIRS
  )
