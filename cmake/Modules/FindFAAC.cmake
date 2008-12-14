# - Try to find FAAC
# Once done, this will define
#
#  FAAC_FOUND - system has FAAC
#  FAAC_INCLUDE_DIRS - the FAAC include directories
#  FAAC_LIBRARIES - link these to use FAAC
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

find_path(FAAC_INCLUDE_DIR
  NAMES faac.h
)

find_library(FAAC_LIBRARY
  NAMES faac
)

set(FAAC_PROCESS_INCLUDES FAAC_INCLUDE_DIR)
set(FAAC_PROCESS_LIBS FAAC_LIBRARY)
libfind_process(FAAC)

