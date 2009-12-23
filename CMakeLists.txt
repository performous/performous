project(Performous CXX)
cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)
set(PROJECT_VERSION "0.4.0+")

# Avoid source tree pollution
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files already created:\nrm -rf CMakeCache.txt CMakeFiles")
endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

include(cmake/performous-packaging.cmake)

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
find_package(Gettext)
if(Gettext_FOUND)
	if(WIN32)
		SET(LOCALE_DIR "../locale")
	endif(WIN32)
	
	if(NOT LOCALE_DIR)
		message(QUIET "Set LOCALE_DIR to path to get Gettext working")
	else()
		add_subdirectory(lang)
	endif(NOT LOCALE_DIR)
endif(Gettext_FOUND)

set(LIB_SUFFIX "" CACHE STRING "Optional suffix to use on lib folders (e.g. 64 for lib64)")

option(ENABLE_EDITOR "Enable experimental song editor (doesn't work)" OFF)

if (ENABLE_EDITOR)
  add_subdirectory(editor)
endif (ENABLE_EDITOR)

option(ENABLE_TOOLS "Enable extra tools (e.g. Singstar ripper)" ON)

if (ENABLE_TOOLS)
  add_subdirectory(tools)
endif (ENABLE_TOOLS)

if(WIN32)
	option(ENABLE_VERSIONING "Add number version information. This allows to add version information to exe. Be carefull, in beta (and a lot useless)" OFF)
endif(WIN32)