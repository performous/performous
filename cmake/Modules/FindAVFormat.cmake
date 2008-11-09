# - Try to find FFMPEG libavformat
# Once done, this will define
#
#  AVFormat_FOUND - the library is available
#  AVFormat_INCLUDE_DIRS - the include directories
#  AVFormat_LIBRARIES - the libraries
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

libfind_package(AVFormat AVCodec)

libfind_pkg_check_modules(AVFormat_PKGCONF libavformat)

find_path(AVFormat_INCLUDE_DIR
  NAMES avformat.h
  PATHS ${AVFormat_PKGCONF_INCLUDE_DIRS}
  PATH_SUFFIXES ffmpeg ffmpeg/avformat avformat
)

find_library(AVFormat_LIBRARY
  NAMES avformat
  PATHS ${AVFormat_PKGCONF_LIBRARY_DIRS}
)

set(AVFormat_PROCESS_INCLUDES AVFormat_INCLUDE_DIR AVCodec_INCLUDE_DIRS)
set(AVFormat_PROCESS_LIBS AVFormat_LIBRARY AVCodec_LIBRARIES)
libfind_process(AVFormat)

