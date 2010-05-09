# - Try to find OpenCV
# Once done, this will define
#
#  OpenCV_FOUND - system has OpenCV
#  OpenCV_INCLUDE_DIRS - the OpenCV include directories
#  OpenCV_LIBRARIES - link these to use OpenCV
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)


find_path(OpenCV_INCLUDE_DIR
  NAMES cv.h cv.hpp
  PATHS ${OpenCV_INCLUDE_DIRS}
  PATH_SUFFIXES opencv
)

find_library(OpenCV_LIBRARY
  NAMES cv highgui highgui4 libcv.a OpenCV opencv CV
  PATHS ${OpenCV_PKGCONF_LIBRARY_DIRS}
)

set(OpenCV_PROCESS_INCLUDES OpenCV_INCLUDE_DIR)
set(OpenCV_PROCESS_LIBS OpenCV_LIBRARY)
libfind_process(OpenCV)

