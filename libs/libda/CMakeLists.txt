project(libda)
cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

set(LibDA_VERSION_MAJOR 1)
set(LibDA_VERSION_MINOR 0)
set(LibDA_VERSION "${LibDA_VERSION_MAJOR}.${LibDA_VERSION_MINOR}")

# Avoid source tree pollution
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files already created:\nrm -rf CMakeCache.txt CMakeFiles")
endif()

# Add a sensible build type default and warning because empty means no optimization and no debug info.
if(NOT CMAKE_BUILD_TYPE)
	message("WARNING: CMAKE_BUILD_TYPE is not defined!\n         Defaulting to CMAKE_BUILD_TYPE=Release. Use ccmake to set a proper value.")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set(LIB_SUFFIX "" CACHE STRING "Optional suffix to use on lib folders (e.g. 64 for lib64)")
set(LibDA_CMAKE_DIR share/libda/cmake CACHE STRING "Relative path (from prefix) where to put CMake configuration files")
set(LibDA_PLUGIN_DIR "lib${LIB_SUFFIX}/libda")

include_directories(include)

FILE(GLOB HEADER_FILES "include/libda/*")
install(FILES ${HEADER_FILES} DESTINATION include/libda)

if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -Wextra")
	if(NOT WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	endif(NOT WIN32)
endif(CMAKE_COMPILER_IS_GNUCXX)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.36 REQUIRED COMPONENTS thread date_time)
include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(cmake)

