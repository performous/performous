project(libda)
cmake_minimum_required(VERSION 2.6)
cmake_policy(VERSION 2.6)

# Add a sensible build type default and warning because empty means no optimization and no debug info.
if(NOT CMAKE_BUILD_TYPE)
	message("WARNING: CMAKE_BUILD_TYPE is not defined!\n         Defaulting to CMAKE_BUILD_TYPE=Release. Use ccmake to set a proper value.")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
endif()

set(LIB_SUFFIX "" CACHE STRING "Optional suffix to use on lib folders (e.g. 64 for lib64)")

set(LIBDA_PLUGIN_DIR "lib${LIB_SUFFIX}/libda")

include_directories(include)

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


