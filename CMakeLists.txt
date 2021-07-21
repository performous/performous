cmake_minimum_required(VERSION 3.6)
project(Performous CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(GetGitRevisionDescription)
git_describe(PROJECT_VERSION --tags)
if(NOT PROJECT_VERSION)
	set(PROJECT_VERSION "1.1+")
endif()

message("Building ${PROJECT_NAME} ${PROJECT_VERSION}")

# Avoid source tree pollution
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files already created:\nrm -rf CMakeCache.txt CMakeFiles")
endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

# Add a sensible build type default and warning because empty means no optimization and no debug info.
if(NOT CMAKE_BUILD_TYPE)
	message("WARNING: CMAKE_BUILD_TYPE is not defined!\n         Defaulting to CMAKE_BUILD_TYPE=RelWithDebInfo. Use ccmake to set a proper value.")
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_definitions("-DNDEBUG")
endif()

if (USE_BOOST_REGEX)
        add_definitions(-DUSE_BOOST_REGEX)
endif()

option(USE_BOOST_FS "Whether to use boost filsystem instead of c++17" FALSE)

if (APPLE)
	if (${CMAKE_OSX_DEPLOYMENT_TARGET})
		message (STATUS "Checking macOS deployment target... ${CMAKE_OSX_DEPLOYMENT_TARGET}")
		if (${CMAKE_OSX_DEPLOYMENT_TARGET} VERSION_GREATER_EQUAL 10.15)
			message (STATUS "Targetting macOS 10.15 or newer, enabling std::filesystem")
                else ()
			set(USE_BOOST_FS TRUE)
		endif()
	else()
		execute_process(COMMAND /usr/bin/sw_vers "-productVersion" OUTPUT_VARIABLE HOST_MACOS_VERSION)
		message (STATUS "macOS deployment target not set; checking host... ${HOST_MACOS_VERSION}")
		if (${HOST_MACOS_VERSION} VERSION_LESS 10.15)
			set(USE_BOOST_FS TRUE)
		endif()
	endif()
endif()

# Note: AUTO_PTR is for Boost which won't compile with C++17 otherwise (fixed in Boost git but not in 1.66)
add_compile_options(-Wall -Wextra -D_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR $<$<CXX_COMPILER_ID:gcc>:-fcx-limited-range>)

if(WIN32)
	set(CMAKE_CXX_FLAGS "-DWIN32_LEAN_AND_MEAN ${CMAKE_CXX_FLAGS}")
	set(SHARE_INSTALL_DEFAULT ".")
	set(LOCALE_DIR_DEFAULT "locale")
else()
	set(SHARE_INSTALL_DEFAULT "share/games/performous")
	set(LOCALE_DIR_DEFAULT "share/locale")
endif()

set(SHARE_INSTALL "${SHARE_INSTALL_DEFAULT}" CACHE STRING "Data file install path. Must be a relative path (from CMAKE_INSTALL_PREFIX), with no trailing slash.")
mark_as_advanced(SHARE_INSTALL)

set(LOCALE_DIR "${LOCALE_DIR_DEFAULT}" CACHE STRING "Locale file install path. Must be a relative path (from CMAKE_INSTALL_PREFIX), with no trailing slash.")
mark_as_advanced(LOCALE_DIR)

find_package(Gettext)
if(Gettext_FOUND)
	message(STATUS "Localization enabled: Building and installing .mo files")
	add_subdirectory(lang)
else()
	message(STATUS "Localization disabled: Gettext tools (msgfmt) missing")
endif()

include(FetchContent)
FetchContent_Declare(ced-sources
  GIT_REPOSITORY https://github.com/performous/compact_enc_det.git
  #https://github.com/google/compact_enc_det.git
  GIT_TAG        master
  SOURCE_DIR ced-src
)

FetchContent_MakeAvailable(ced-sources)

option(USE_SELF_BUILT_AUBIO "Use custom aubio local build instead of using system lib (if available)" FALSE)

find_package(Aubio REQUIRED)

add_subdirectory(data)

add_subdirectory(game)
add_subdirectory(docs)

if(WIN32)
	install(FILES win32/ConfigureSongDirectory.bat DESTINATION .)
endif()

if(NOT APPLE)
  # uninstall target
  if(NOT TARGET uninstall)
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake"
      IMMEDIATE @ONLY)

    add_custom_target(uninstall
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake)
  endif()
endif()

#CPACK variables
include(CPack)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Niek Nooijens")
set(CPACK_PACKAGE_CONTACT "nieknooijens@gmail.com")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libavcodec54, libavformat54, libswresample2, libsdl2-2.0-0, libportaudiocpp0, libboost-all-dev, libswscale2, libxml++2.6-2, libopencv-dev, librsvg2-2, libpango-1.0-0, libcairo2, libjpeg8, libpng12-0, libportmidi0, libicu-dev, libglm-dev libopenblas-dev libfftw3-dev")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION_SUMMARY "A free music rhythm game similar to SingStar, Guitar Hero and DDR")
set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${CMAKE_BINARY_DIR};${PROJECT_NAME};ALL;/")
