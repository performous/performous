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

libfind_pkg_detect(Fmt fmt FIND_PATH fmt/core.h fmt/format.h FIND_LIBRARY fmt)
set(Fmt_VERSION ${Fmt_PKGCONF_VERSION})
libfind_process(Fmt)

if (NOT Fmt_FOUND)
    message("Building fmt from sources...")
    include(FetchContent)
    FetchContent_Declare(fmt-sources
      GIT_REPOSITORY https://github.com/fmtlib/fmt.git
      GIT_TAG        b6f4ceaed0a0a24ccf575fab6c56dd50ccf6f1a9
      SOURCE_DIR fmt-src
    )
    FetchContent_MakeAvailable(fmt-sources)
endif()