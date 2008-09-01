project(ultrastar-ng)
cmake_minimum_required(VERSION 2.6)

add_subdirectory(audio)

FILE(GLOB SOURCE_FILES "src/*.cpp")
FILE(GLOB HEADER_FILES "src/*.hh")

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES})

# Libraries

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.34 REQUIRED)
include_directories(${BOOST_INCLUDE_DIR})
set(LIBS ${LIBS} ${BOOST_LIBRARIES})

find_package(Cairo REQUIRED)
include_directories(${CAIRO_INCLUDE_DIR})
set(LIBS ${LIBS} ${CAIRO_LIBRARIES})

add_executable(ultrastarng ${SOURCES})
target_link_libraries(ultrastarng da ${BOOST_LIBRARIES})

install(TARGETS ultrastarng DESTINATION bin)

