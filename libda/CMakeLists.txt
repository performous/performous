find_package(Boost 1.34 REQUIRED COMPONENTS thread filesystem)

project(libda)
cmake_minimum_required(VERSION 2.6)

set(LIBDA_PLUGIN_DIR "${INSTALL_LIBRARY_DIR}/libda")

include_directories(..) # to make #include <libda/*> work

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.34 REQUIRED COMPONENTS thread filesystem)

include_directories(${Boost_INCLUDE_DIR})
add_library(da audio.cpp)
target_link_libraries(da ${Boost_LIBRARIES})
install(TARGETS da DESTINATION lib)

add_subdirectory(plugins)

