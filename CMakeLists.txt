project(ultrastar-ng)
cmake_minimum_required(VERSION 2.6)

set(SHARE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/share/ CACHE PATH "Base directory for files which go to share/" FORCE)

add_subdirectory(src)
add_subdirectory(audio)
add_subdirectory(themes)

