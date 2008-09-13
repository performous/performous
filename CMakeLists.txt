project(ultrastar-ng)
cmake_minimum_required(VERSION 2.6)

set(CPACK_GENERATOR "TBZ2;TGZ")
set(CPACK_SOURCE_GENERATOR "TBZ2;TGZ")
set(CPACK_PACKAGE_NAME "UltraStar-NG")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A karaoke game with pitch detection and scoring, similar to Singstar games. Supports songs in Ultrastar format.")
set(CPACK_PACKAGE_CONTACT "http://ultrastar-ng.sf.net/")
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

if(NOT INSTALL_LIBRARY_DIR)
  set(INSTALL_LIBRARY_DIR ${CMAKE_INSTALL_PREFIX}/lib/ CACHE PATH "Base directory for libraries" FORCE)
endif(NOT INSTALL_LIBRARY_DIR)

if(DATA_INSTALL_DIR)
  set(SHARE_INSTALL_PREFIX ${DATA_INSTALL_DIR} CACHE PATH "Base directory for files which go to share/" FORCE)
else(DATA_INSTALL_DIR)
  set(SHARE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/share/ CACHE PATH "Base directory for files which go to share/" FORCE)
endif(DATA_INSTALL_DIR)

add_subdirectory(libda)
add_subdirectory(themes)
add_subdirectory(data)
add_subdirectory(game)

