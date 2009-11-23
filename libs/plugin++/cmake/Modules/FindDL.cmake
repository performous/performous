# - Try to find dynamic linker library. Does nothing on Windows
# Once done, this will define
#
#  DL_FOUND - system has dl (always set on Windows)
#  DL_INCLUDE_DIRS - dlfcn.h include dir
#  DL_LIBRARIES - link these to use dl
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

if(WIN32)
	set(DL_FOUND TRUE)
else()
	include(LibFindMacros)

	find_path(DL_INCLUDE_DIR NAMES dlfcn.h)
	find_library(DL_LIBRARY NAMES dl)

	set(DL_PROCESS_INCLUDES DL_INCLUDE_DIR)
	set(DL_PROCESS_LIBS DL_LIBRARY)
	libfind_process(DL)
endif()

