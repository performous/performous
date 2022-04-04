# - Try to find Fmt
# Once done, this will define
#
#  Fmt_FOUND - system has fmt
#  Fmt_INCLUDE_DIRS - the fmt include directories
#  Fmt_LIBRARIES - link these to use fmt
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_pkg_detect(Fmt fmt FIND_PATH fmt/core.h FIND_LIBRARY fmt)
set(Fmt_VERSION ${Fmt_PKGCONF_VERSION})
libfind_process(Fmt)
