cmake_minimum_required(VERSION 2.6)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.34 REQUIRED COMPONENTS thread filesystem)

include_directories(${Boost_INCLUDE_DIRS})
add_library(da SHARED audio.cpp)
target_link_libraries(da plugin++ ${Boost_LIBRARIES})
install(TARGETS da DESTINATION lib${LIB_SUFFIX})
