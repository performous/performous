project(Performous)
cmake_minimum_required(VERSION 2.4)

set(CPACK_GENERATOR "TBZ2;TGZ")
set(CPACK_SOURCE_GENERATOR "TBZ2;TGZ")
set(CPACK_PACKAGE_NAME "performous")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A karaoke game with pitch detection and scoring, similar to Singstar games. Supports songs in Ultrastar format.")
set(CPACK_PACKAGE_CONTACT "http://performous.org/")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 3)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_SOURCE_IGNORE_FILES
   "/.cvsignore"
   "/songs/"
   "/build/"
   "/.svn/"
)
set(CPACK_DEBIAN_PACKAGE_DEPENDS libsdl1.2 libsdl-image1.2 libcairo2 librsvg2 libasound2 libjack libboost-thread libboost-serialization libboost-program-options libboost-regex libboost-filesystem libavcodec libavformat libswscale libmagick++9 libsamplerate0 libpulse libxml++2.6)
include(CPack)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/Modules/)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)


set(INSTALL_LIBRARY_DIR ${CMAKE_INSTALL_PREFIX}/lib/ CACHE PATH "Library installation directory")
set(SHARE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/share/performous/ CACHE PATH "Data file installation directory")

add_subdirectory(libda)
add_subdirectory(themes)
add_subdirectory(data)
add_subdirectory(game)

