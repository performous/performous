project(Plugin++)
cmake_minimum_required(VERSION 2.4)
cmake_policy(VERSION 2.6)

include_directories(include)

set(LIB_SUFFIX "" CACHE STRING "Optional suffix to use on lib folders (e.g. 64 for lib64)")

if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -Wextra")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_library(plugin++ SHARED src/dll.cc src/execname.cc src/loader.cc)

find_package(Boost 1.36 REQUIRED COMPONENTS filesystem)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(plugin++ ${Boost_LIBRARIES})

add_definitions("-DLIB_SUFFIX=\"${LIB_SUFFIX}\"")

if(NOT WIN32)
  target_link_libraries(plugin++ dl)
endif()

set_target_properties(plugin++ PROPERTIES INSTALL_NAME_DIR @rpath)
install(TARGETS plugin++ DESTINATION lib${LIB_SUFFIX})

FILE(GLOB HEADER_FILES "include/plugin++/*")
install(FILES ${HEADER_FILES} DESTINATION include/plugin++)

