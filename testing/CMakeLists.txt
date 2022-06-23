cmake_minimum_required(VERSION 3.15)

file(GLOB SOURCE_FILES "*.cc")
file(GLOB HEADER_FILES "*.hh")
set(GAME_SOURCES 
    "../game/configitem.cc"
    "../game/dynamicnotegraphscaler.cc"
    "../game/fixednotegraphscaler.cc"
    "../game/musicalscale.cc"
    "../game/notes.cc"
    "../game/notegraphscalerfactory.cc"
)

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES} ${GAME_SOURCES})

add_executable(performous_test ${SOURCES})

find_package(Boost 1.55 REQUIRED COMPONENTS program_options iostreams system locale)
target_include_directories(performous_test PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(performous_test PRIVATE ${Boost_LIBRARIES})

find_package(fmt REQUIRED CONFIG)
target_link_libraries(performous_test PRIVATE fmt::fmt)

target_include_directories(performous_test PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(performous_test PRIVATE GTest::GTest GTest::Main)

target_include_directories(performous_test PRIVATE "..")
