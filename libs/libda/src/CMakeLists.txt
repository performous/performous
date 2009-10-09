cmake_minimum_required(VERSION 2.6)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.34 REQUIRED COMPONENTS thread filesystem)

if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, adding compile flags")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++98 -pedantic -Wall -Wextra -fvisibility=hidden")
endif(CMAKE_COMPILER_IS_GNUCXX)

include_directories(${Boost_INCLUDE_DIRS})
add_library(da audio.cpp)
target_link_libraries(da plugin++ ${Boost_LIBRARIES})
install(TARGETS da DESTINATION lib${LIB_SUFFIX})
