project(Performous)
cmake_minimum_required(VERSION 2.4)

# Avoid source tree pollution
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files already created:\nrm -rf CMakeCache.txt CMakeFiles")
endif(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)

set(CPACK_GENERATOR "TBZ2;DEB")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_PACKAGE_NAME "Performous")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A karaoke game with pitch detection and scoring, similar to Singstar games. Previously known as UltraStar-NG. Supports songs in Ultrastar format.")
set(CPACK_PACKAGE_CONTACT "http://performous.org/")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 3)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_SOURCE_IGNORE_FILES
   "/.cvsignore"
   "/songs/"
   "/build/"
   "/.svn/"
   "/osx-utils/"
   "/portage-overlay/"
)
# These deps are for Ubuntu 8.10
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl1.2debian, libsdl-image1.2, libcairo2, librsvg2-2, libasound2, libjack0, libboost-thread1.34.1, libboost-serialization1.34.1, libboost-program-options1.34.1, libboost-regex1.34.1, libboost-filesystem1.34.1, libavcodec51, libavformat52, libswscale0, libmagick++10, libsamplerate0, libpulse0, libxml++2.6-2")
set(CPACK_DEBIAN_PACKAGE_PRIORITY extra)
set(CPACK_DEBIAN_PACKAGE_SECTION universe/games)
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS ultrastar-songs)
include(CPack)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Add a sensible build type default and warning because empty means no optimization and no debug info.
if(NOT CMAKE_BUILD_TYPE)
	message("WARNING: CMAKE_BUILD_TYPE is not defined!\n         Defaulting to CMAKE_BUILD_TYPE=RelWithDebInfo. Use ccmake to set a proper value.")
	SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)


set(SHARE_INSTALL "share/performous")

add_subdirectory(libda)
add_subdirectory(themes)
add_subdirectory(data)
add_subdirectory(game)

if(NOT "${CMAKE_INSTALL_PREFIX}" MATCHES "^/usr/")
	set(PERFORMOUS_THEME_PATH "${CMAKE_INSTALL_PREFIX}/${SHARE_INSTALL}/themes")
	set(PERFORMOUS_PLUGIN_PATH "${CMAKE_INSTALL_PREFIX}/lib/libda")
	set(PERFORMOUS_EXECUTABLE "${CMAKE_INSTALL_PREFIX}/bin/performous")
	configure_file("${PROJECT_SOURCE_DIR}/cmake/performous.sh.cmake" "${PROJECT_BINARY_DIR}/performous.sh" ESCAPE_QUOTES @ONLY)
	install(PROGRAMS "${PROJECT_BINARY_DIR}/performous.sh" DESTINATION bin)
	install(SCRIPT "${PROJECT_SOURCE_DIR}/cmake/performous-launcher.cmake")
endif(NOT "${CMAKE_INSTALL_PREFIX}" MATCHES "^/usr/")

