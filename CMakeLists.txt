project(Performous CXX)
cmake_minimum_required(VERSION 2.4)
set(PROJECT_VERSION "0.3.1+")

# Avoid source tree pollution
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files already created:\nrm -rf CMakeCache.txt CMakeFiles")
endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

if(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 5)
  include("${CMAKE_SOURCE_DIR}/cmake/performous-packaging.cmake")
else(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 5)
  message("Automatic packaging support disabled (CMake 2.6 or later required)")
endif(${CMAKE_MAJOR_VERSION} EQUAL 2 AND ${CMAKE_MINOR_VERSION} GREATER 5)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Add a sensible build type default and warning because empty means no optimization and no debug info.
if(NOT CMAKE_BUILD_TYPE)
	message("WARNING: CMAKE_BUILD_TYPE is not defined!\n         Defaulting to CMAKE_BUILD_TYPE=RelWithDebInfo. Use ccmake to set a proper value.")
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(WIN32)
	set(SHARE_INSTALL "." CACHE STRING "Data file install path. Must be a relative path (from CMAKE_INSTALL_PREFIX), with no trailing slash.")
else(WIN32)
	set(SHARE_INSTALL "share/games/performous" CACHE STRING "Data file install path. Must be a relative path (from CMAKE_INSTALL_PREFIX), with no trailing slash.")
endif(WIN32)

mark_as_advanced(SHARE_INSTALL)

add_subdirectory(libs)
add_subdirectory(themes)
add_subdirectory(data)
add_subdirectory(game)
add_subdirectory(docs)

option(ENABLE_TOOLS "Enable extra tools (e.g. Singstar ripper)" ON)
option(ENABLE_SILKYSTRINGS "Enable Silky Strings (Guitar Hero clone)" ON)

if (ENABLE_TOOLS)
  add_subdirectory(tools)
endif (ENABLE_TOOLS)

if (ENABLE_SILKYSTRINGS)
  add_subdirectory(silkystrings)
endif (ENABLE_SILKYSTRINGS)

if(NOT WIN32)
	if(NOT "${CMAKE_INSTALL_PREFIX}" MATCHES "^(/usr|/usr/local)$")
		message(STATUS "Non-standard installation prefix. Configuring performous.sh.")
		set(LAUNCHER_SCRIPT TRUE)
	endif(NOT "${CMAKE_INSTALL_PREFIX}" MATCHES "^(/usr|/usr/local)$")

	if(NOT "${SHARE_INSTALL}" MATCHES "^(share/performous|share/games/performous)$")
		message(STATUS "Non-standard data file install path. Configuring performous.sh.")
		set(LAUNCHER_SCRIPT TRUE)
	endif(NOT "${SHARE_INSTALL}" MATCHES "^(share/performous|share/games/performous)$")
endif(NOT WIN32)

if(LAUNCHER_SCRIPT)
	set(FULL_PREFIX ${CMAKE_INSTALL_PREFIX})
	if(UNIX AND NOT "${FULL_PREFIX}" MATCHES "^/")
	  set(FULL_PREFIX "${CMAKE_BINARY_DIR}/${FULL_PREFIX}")
	endif(UNIX AND NOT "${FULL_PREFIX}" MATCHES "^/")	 
	set(PERFORMOUS_CONFIG_SCHEMA "${FULL_PREFIX}/${SHARE_INSTALL}/config/performous.xml")
	set(PERFORMOUS_PLUGIN_PATH "${FULL_PREFIX}/lib${LIB_SUFFIX}/libda")
	set(PERFORMOUS_EXECUTABLE "${FULL_PREFIX}/bin/performous")
	configure_file("${PROJECT_SOURCE_DIR}/cmake/performous.sh.cmake" "${PROJECT_BINARY_DIR}/performous.sh" ESCAPE_QUOTES @ONLY)
	install(PROGRAMS "${PROJECT_BINARY_DIR}/performous.sh" DESTINATION bin)
	install(SCRIPT "${PROJECT_SOURCE_DIR}/cmake/performous-launcher.cmake")
endif(LAUNCHER_SCRIPT)

