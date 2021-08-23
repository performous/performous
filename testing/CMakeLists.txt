cmake_minimum_required(VERSION 3.15)

file(GLOB SOURCE_FILES "*.cc")
file(GLOB HEADER_FILES "*.hh")

include_directories(${GTEST_INCLUDE_DIRS} "..")

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES})

add_executable(performous_test ${SOURCES})


target_include_directories(performous_test SYSTEM PRIVATE ${GTEST_INCLUDE_DIRS})
target_link_libraries(performous_test GTest::GTest GTest::Main)
