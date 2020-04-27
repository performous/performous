cmake_minimum_required(VERSION 3.6)
project(Performous CXX C)

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
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_definitions("-DNDEBUG")
endif()

if (USE_BOOST_REGEX)
        add_definitions(-DUSE_BOOST_REGEX)
endif()

if(WIN32)
	set(CMAKE_CXX_FLAGS "-DWIN32_LEAN_AND_MEAN ${CMAKE_CXX_FLAGS}")
	set(SHARE_INSTALL_DEFAULT ".")
	SET(LOCALE_DIR_DEFAULT "locale")
else()
	set(SHARE_INSTALL_DEFAULT "share/games/performous")
	SET(LOCALE_DIR_DEFAULT "share/locale")
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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ced)

if (APPLE)
	set_property(GLOBAL PROPERTY BLA_VENDOR APPLE)
	set(ACCELERATE_COMMAND "--enable-accelerate")
	set(AUBIO_CFLAGS "${CMAKE_C_FLAGS} -iframeworkwithsysroot /System/Library/Frameworks/ -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} -isysroot '${CMAKE_OSX_SYSROOT}/'")
else()
	set(ACCELERATE_COMMAND "--enable-atlas")
        set_property(GLOBAL PROPERTY BLA_VENDOR OpenBLAS)
	set(AUBIO_CFLAGS "${CMAKE_C_FLAGS}")
endif()

#include(ExternalProject)
#ExternalProject_Add(build-aubio-from-sources
#	SOURCE_DIR ${CMAKE_SOURCE_DIR}/3rdparty/aubio/
#        BINARY_DIR ${CMAKE_BINARY_DIR}/3rdparty/aubio-build/
#        INSTALL_DIR ${CMAKE_BINARY_DIR}/3rdparty/aubio-install/
#        UPDATE_COMMAND ""
#	TMP_DIR ${CMAKE_BINARY_DIR}/3rdparty/aubio-build/tmp/
#	CONFIGURE_COMMAND ""
#	BUILD_COMMAND ${CMAKE_COMMAND} -E env CFLAGS=${AUBIO_CFLAGS} ${CMAKE_SOURCE_DIR}/3rdparty/aubio/waf
#                  ${ACCELERATE_COMMAND} --disable-tests --notests --verbose
#                  --enable-fftw3f --disable-sndfile --disable-avcodec --disable-double
#                  --disable-samplerate --disable-docs --disable-wavread --disable-wavwrite
#                  --disable-tests --notests --disable-examples --disable-apple-audio
#                  --disable-fat --disable-jack --libdir=lib
#                  --prefix=${CMAKE_BINARY_DIR}/3rdparty/aubio-install/
#                  --top=${CMAKE_SOURCE_DIR}/3rdparty/aubio/
#                  --out=${CMAKE_BINARY_DIR}/3rdparty/aubio-build/ configure build install
#	INSTALL_COMMAND ""
#)
#
#ExternalProject_Get_Property(build-aubio-from-sources INSTALL_DIR)
#set (AUBIO_INSTALL_DIR ${INSTALL_DIR})
#add_library(aubio STATIC IMPORTED)
add_subdirectory("3rdparty/aubio")

target_include_directories(aubio INTERFACE
	3rdparty/aubio/src
)

#get_target_property(AUBIO_INSTALL_DIR aubio LIBRARY_OUTPUT_DIRECTORY)
#message (WARNING "AUBIO OUTPUT DIRECTORY: ${AUBIO_INSTALL_DIR}")
#set_target_properties(aubio PROPERTIES IMPORTED_LOCATION ${AUBIO_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}aubio${CMAKE_STATIC_LIBRARY_SUFFIX})
#add_dependencies(aubio build-aubio-from-sources)

add_subdirectory(data)

add_subdirectory(game)
add_subdirectory(3rdparty/ced)
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
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Niek Nooijens")
SET(CPACK_PACKAGE_CONTACT "nieknooijens@gmail.com")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libavcodec54, libavformat54, libswresample2, libsdl2-2.0-0, libportaudiocpp0, libboost-all-dev, libswscale2, libxml++2.6-2, libopencv-dev, librsvg2-2, libpango-1.0-0, libcairo2, libjpeg8, libpng12-0, libportmidi0, libicu-dev, libglm-dev libopenblas-dev libfftw3-dev")
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION_SUMMARY "A free music rhythm game similar to SingStar, Guitar Hero and DDR")
SET(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION})
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
SET(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${CMAKE_BINARY_DIR};${PROJECT_NAME};ALL;/")
