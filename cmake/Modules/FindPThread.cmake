# - Try to find PThread
# Once done, this will define
#
#  PThread_FOUND - system has PThread
#  PThread_LIBRARIES - link these to use PThread
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_check_modules(PThread_PKGCONF PThread)

find_library(PThread_LIBRARY
  NAMES pthreadGC2
  HINTS ${PThread_PKGCONF_LIBRARY_DIRS}
)

set(PThread_PROCESS_LIBS PThread_LIBRARY)
libfind_process(PThread)

