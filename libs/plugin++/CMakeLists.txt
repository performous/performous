project(Plugin++)
cmake_minimum_required(VERSION 2.4)

include_directories(include)

if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -pedantic -Wall -Wextra -fvisibility=hidden -fvisibility-inlines-hidden")
endif(CMAKE_COMPILER_IS_GNUCXX)

if (WIN32)
  add_library(plugin++ STATIC src/dll-windows.cpp)
else (WIN32)
  add_library(plugin++ STATIC src/dll-posix.cpp)
endif (WIN32)

install(TARGETS plugin++ DESTINATION lib${LIB_SUFFIX})

FILE(GLOB HEADER_FILES "include/plugin++/*")
install(FILES ${HEADER_FILES} DESTINATION include/plugin++)

