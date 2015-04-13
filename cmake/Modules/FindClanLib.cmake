# - Try to find ClanLib
# Example usage: find_package(ClanLib 0.8 REQUIRED COMPONENTS Core App Display GL Sound)
#
# Once done, this will define
#
#  ClanLib_FOUND - system has ClanLib (all the components requested)
#  ClanLib_INCLUDE_DIRS - the ClanLib include directories
#  ClanLib_LIBRARIES - link these to use ClanLib
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

if (ClanLib_FIND_VERSION_MAJOR AND ClanLib_FIND_VERSION_MINOR)
  set(ClanLib_VERSION "${ClanLib_FIND_VERSION_MAJOR}.${ClanLib_FIND_VERSION_MINOR}")
else (ClanLib_FIND_VERSION_MAJOR AND ClanLib_FIND_VERSION_MINOR)
  set(ClanLib_VERSION "0.8")
endif (ClanLib_FIND_VERSION_MAJOR AND ClanLib_FIND_VERSION_MINOR)

libfind_pkg_check_modules(ClanLib_PKGCONF clanCore-${ClanLib_VERSION})

find_path(ClanLib_INCLUDE_DIR
  NAMES ClanLib/core.h
  HINTS ${ClanLib_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES ClanLib-${ClanLib_VERSION}
)

# Extract the actual version number
if (ClanLib_INCLUDE_DIR)
  file(READ "${ClanLib_INCLUDE_DIR}/ClanLib/core.h" _ClanLib_CORE_H_CONTENTS)
  set(_ClanLib_VERSION_REGEX ".*#define CL_VERSION_STRING \"([^\n]*)\".*")
  if ("${_ClanLib_CORE_H_CONTENTS}" MATCHES "${_ClanLib_VERSION_REGEX}")
    string(REGEX REPLACE "${_ClanLib_VERSION_REGEX}" "\\1" ClanLib_VERSION "${_ClanLib_CORE_H_CONTENTS}")
  endif ("${_ClanLib_CORE_H_CONTENTS}" MATCHES "${_ClanLib_VERSION_REGEX}")
endif (ClanLib_INCLUDE_DIR)

foreach(COMPONENT ${ClanLib_FIND_COMPONENTS})
  find_library(ClanLib_${COMPONENT}_LIBRARY
    NAMES clan${COMPONENT}
    HINTS ${ClanLib_PKGCONF_LIBRARY_DIRS}
  )
  set(ClanLib_PROCESS_LIBS ${ClanLib_PROCESS_LIBS} ClanLib_${COMPONENT}_LIBRARY)
endforeach(COMPONENT)

set(ClanLib_PROCESS_INCLUDES ClanLib_INCLUDE_DIR)
libfind_process(ClanLib)

