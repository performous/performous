project(Plugin++)
cmake_minimum_required(VERSION 2.6)

set(Plugin++_VERSION_MAJOR 1)
set(Plugin++_VERSION_MINOR 0)
set(Plugin++_VERSION "${Plugin++_VERSION_MAJOR}.${Plugin++_VERSION_MINOR}")

# Avoid source tree pollution
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files already created:\nrm -rf CMakeCache.txt CMakeFiles")
endif()

include("${CMAKE_SOURCE_DIR}/cmake/plugin++-packaging.cmake")

# Add a sensible build type default and warning because empty means no optimization and no debug info.
if(NOT CMAKE_BUILD_TYPE)
	message("WARNING: CMAKE_BUILD_TYPE is not defined!\n         Defaulting to CMAKE_BUILD_TYPE=Release. Use ccmake to set a proper value.")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include_directories(include)

set(Plugin++_CMAKE_DIR share/plugin++/cmake CACHE STRING "Relative path (from prefix) where to put CMake configuration files")
set(LIB_SUFFIX "" CACHE STRING "Optional suffix to use on lib folders (e.g. 64 for lib64)")

if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -Wextra")
endif()

add_library(plugin++ SHARED src/dll.cc src/execname.cc src/loader.cc)

find_package(DL REQUIRED)
include_directories(${DL_INCLUDE_DIRS})
target_link_libraries(plugin++ ${DL_LIBRARIES})

find_package(Boost 1.36 REQUIRED COMPONENTS filesystem)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(plugin++ ${Boost_LIBRARIES})

add_definitions("-DLIB_SUFFIX=\"${LIB_SUFFIX}\"")

set_target_properties(plugin++ PROPERTIES
	INSTALL_NAME_DIR @rpath
	VERSION "${Plugin++_VERSION}"
	SOVERSION "${Plugin++_VERSION_MAJOR}"
	DEBUG_POSTFIX -d
	INSTALL_RPATH_USE_LINK_PATH TRUE
)

install(TARGETS plugin++ EXPORT Plugin++ DESTINATION lib${LIB_SUFFIX})

FILE(GLOB HEADER_FILES "include/plugin++/*")
install(FILES ${HEADER_FILES} DESTINATION include/plugin++)

# Install/generate CMake files for finding Plugin++
add_subdirectory(cmake)

