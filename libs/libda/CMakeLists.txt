project(libda)
cmake_minimum_required(VERSION 2.6)

set(LIBDA_PLUGIN_DIR "lib${LIB_SUFFIX}/libda")

include_directories(include)
include_directories(../plugin++/include)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.34 REQUIRED COMPONENTS thread filesystem)

include_directories(${Boost_INCLUDE_DIR})
add_library(da src/audio.cpp)
target_link_libraries(da plugin++ ${Boost_LIBRARIES})
install(TARGETS da DESTINATION lib${LIB_SUFFIX})

FILE(GLOB HEADER_FILES "include/libda/*")
install(FILES ${HEADER_FILES} DESTINATION include/libda)

add_subdirectory(plugins)

