project(Plugin++)
cmake_minimum_required(VERSION 2.4)

include_directories(include)

if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -Wextra")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_library(plugin++ SHARED src/dll.cc src/execname.cc src/loader.cc)

find_package(Boost 1.36 REQUIRED COMPONENTS filesystem)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(plugin++ ${Boost_LIBRARIES})

if(NOT WIN32)
  target_link_libraries(plugin++ dl)
endif()

install(TARGETS plugin++ DESTINATION lib${LIB_SUFFIX})

FILE(GLOB HEADER_FILES "include/plugin++/*")
install(FILES ${HEADER_FILES} DESTINATION include/plugin++)

