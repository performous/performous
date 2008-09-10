project(ultrastar-ng)
cmake_minimum_required(VERSION 2.6)

if(DATA_INSTALL_DIR)
  set(SHARE_INSTALL_PREFIX ${DATA_INSTALL_DIR} CACHE PATH "Base directory for files which go to share/" FORCE)
else(DATA_INSTALL_DIR)
  set(SHARE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/share/ CACHE PATH "Base directory for files which go to share/" FORCE)
endif(DATA_INSTALL_DIR)


add_subdirectory(src)
add_subdirectory(audio)
add_subdirectory(themes)

