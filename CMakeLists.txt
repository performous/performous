project(Performous CXX)
cmake_minimum_required(VERSION 2.6)
set(PROJECT_VERSION "0.4-pre4")

# Avoid source tree pollution
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files already created:\nrm -rf CMakeCache.txt CMakeFiles")
endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

include("${CMAKE_SOURCE_DIR}/cmake/performous-packaging.cmake")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Add a sensible build type default and warning because empty means no optimization and no debug info.
if(NOT CMAKE_BUILD_TYPE)
	message("WARNING: CMAKE_BUILD_TYPE is not defined!\n         Defaulting to CMAKE_BUILD_TYPE=RelWithDebInfo. Use ccmake to set a proper value.")
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(WIN32)
	set(SHARE_INSTALL "." CACHE STRING "Data file install path. Must be a relative path (from CMAKE_INSTALL_PREFIX), with no trailing slash.")
else(WIN32)
	if(NOT SHARE_INSTALL)
		set(SHARE_INSTALL "share/games/performous" CACHE STRING "Data file install path. Must be a relative path (from CMAKE_INSTALL_PREFIX), with no trailing slash.")
	endif(NOT SHARE_INSTALL)
endif(WIN32)

mark_as_advanced(SHARE_INSTALL)

# Need to find the headers even if they haven't been installed yet:
include_directories(libs/plugin++/include)
include_directories(libs/libda/include)

add_subdirectory(libs)
add_subdirectory(themes)
add_subdirectory(data)
add_subdirectory(game)
add_subdirectory(docs)

option(ENABLE_EDITOR "Enable experimental song editor (doesn't work)" OFF)

if (ENABLE_EDITOR)
  add_subdirectory(editor)
endif (ENABLE_EDITOR)

option(ENABLE_TOOLS "Enable extra tools (e.g. Singstar ripper)" ON)

if (ENABLE_TOOLS)
  add_subdirectory(tools)
endif (ENABLE_TOOLS)

set(FULL_PREFIX ${CMAKE_INSTALL_PREFIX})
if(UNIX AND NOT "${FULL_PREFIX}" MATCHES "^/")
  set(FULL_PREFIX "${CMAKE_BINARY_DIR}/${FULL_PREFIX}")
endif(UNIX AND NOT "${FULL_PREFIX}" MATCHES "^/")
set(PERFORMOUS_PLUGIN_PATH "${FULL_PREFIX}/lib${LIB_SUFFIX}/libda")
set(PERFORMOUS_EXECUTABLE "${FULL_PREFIX}/bin/performous.bin")
configure_file("${PROJECT_SOURCE_DIR}/cmake/performous.sh.cmake" "${PROJECT_BINARY_DIR}/performous" ESCAPE_QUOTES @ONLY)
install(PROGRAMS "${PROJECT_BINARY_DIR}/performous" DESTINATION bin)
install(SCRIPT "${PROJECT_SOURCE_DIR}/cmake/performous-launcher.cmake")
