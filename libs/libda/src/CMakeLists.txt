cmake_minimum_required(VERSION 2.6)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.34 REQUIRED COMPONENTS thread)
include_directories(${Boost_INCLUDE_DIRS})

find_package(Plugin++ REQUIRED)
include_directories(${Plugin++_INCLUDE_DIRS})

add_library(da SHARED audio.cpp)
target_link_libraries(da plugin++ ${Boost_LIBRARIES} ${Plugin++_LIBRARIES})
set_target_properties(da PROPERTIES INSTALL_NAME_DIR @rpath)

install(TARGETS da DESTINATION lib${LIB_SUFFIX})

