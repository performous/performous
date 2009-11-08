project(libda)
cmake_minimum_required(VERSION 2.6)

set(LIBDA_PLUGIN_DIR "lib${LIB_SUFFIX}/libda")

include_directories(include)
include_directories(../plugin++/include)

FILE(GLOB HEADER_FILES "include/libda/*")
install(FILES ${HEADER_FILES} DESTINATION include/libda)

if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -Wextra")
	if(NOT WIN32)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	endif(NOT WIN32)
endif(CMAKE_COMPILER_IS_GNUCXX)

add_subdirectory(src)
add_subdirectory(plugins)

