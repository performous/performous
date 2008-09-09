project(ultrastar-ng)
cmake_minimum_required(VERSION 2.6)

add_subdirectory(audio)
include_directories(audio)

FILE(GLOB SOURCE_FILES "src/*.cpp")
FILE(GLOB HEADER_FILES "src/*.hh")

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES})

# Libraries

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREAD ON)
find_package(Boost 1.34 REQUIRED COMPONENTS thread)
include_directories(${Boost_INCLUDE_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

find_package(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})
set(LIBS ${LIBS} ${SDL_LIBRARY})

find_package(LibRSVG REQUIRED)
include_directories(${LIBRSVG_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LIBRSVG_LIBRARIES})

find_package(Cairo REQUIRED)
include_directories(${CAIRO_INCLUDE_DIR})
set(LIBS ${LIBS} ${CAIRO_LIBRARIES})

find_package(Pango REQUIRED)
include_directories(${PANGO_INCLUDE_DIRS})
set(LIBS ${LIBS} ${PANGO_LIBRARIES})

find_package(LibXml++ REQUIRED)
include_directories(${LIBXMLPP_INCLUDE_DIRS})
set(LIBS ${LIBS} ${LIBXMLPP_LIBRARIES})

message("${LIBS}")

add_executable(ultrastarng ${SOURCES})
target_link_libraries(ultrastarng da ${LIBS})

install(TARGETS ultrastarng DESTINATION bin)

