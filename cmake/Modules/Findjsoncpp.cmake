# -*- cmake -*-

# - Find JSONCpp
# Find the JSONCpp includes and library
# This module defines
#  JSONCPP_FOUND, System has libjsoncpp.
#  JSONCPP_INCLUDE_DIRS - The libjsoncpp include directories.
#  JSONCPP_LIBRARIES - The libraries needed to use libjsoncpp.
#  JSONCPP_DEFINITIONS - Compiler switches required for using libjsoncpp.

FIND_PACKAGE(PkgConfig)
PKG_CHECK_MODULES(PC_JSONCPP jsoncpp)
SET(JSONCPP_DEFINITIONS ${PC_JSONCPP_CFLAGS_OTHER})

FIND_PATH(JSONCPP_INCLUDE_DIR json/reader.h
  HINTS ${PC_JSONCPP_INCLUDE_DIR} ${PC_JSONCPP_INCLUDE_DIRS}
  PATH_SUFFIXES jsoncpp)

# Get the GCC compiler version
EXEC_PROGRAM(${CMAKE_CXX_COMPILER}
            ARGS ${CMAKE_CXX_COMPILER_ARG1} -dumpversion
            OUTPUT_VARIABLE _gcc_COMPILER_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )

# Try to find a library that was compiled with the same compiler version as we currently use.
FIND_LIBRARY(JSONCPP_LIBRARY
  NAMES libjson_linux-gcc-${_gcc_COMPILER_VERSION}_libmt.so libjsoncpp.so
  HINTS ${PC_JSONCPP_LIBDIR} ${PC_JSONCPP_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib)

SET(JSONCPP_LIBRARIES ${JSONCPP_LIBRARY})
SET(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JSONCPP DEFAULT_MSG
  JSONCPP_LIBRARY JSONCPP_INCLUDE_DIR)

MARK_AS_ADVANCED(JSONCPP_LIBRARY JSONCPP_INCLUDE_DIR)
