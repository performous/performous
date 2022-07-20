cmake_minimum_required(VERSION 3.15)

set(SOURCE_FILES
	"colortest.cc"
	"configitemtest.cc"
	"exceptionwithstacktracetest.cc"
	"fixednotegraphscalertest.cc"
	"notegraphscalerfactorytest.cc"
	"stacktracetest.cc"
	"utiltest.cc"
)
set(GAME_SOURCES
	"../game/color.cc"
	"../game/configitem.cc"
	"../game/dynamicnotegraphscaler.cc"
	"../game/fixednotegraphscaler.cc"
	"../game/musicalscale.cc"
	"../game/notes.cc"
	"../game/notegraphscalerfactory.cc"
	"../game/misc/stacktrace.cc"
	"../game/misc/exceptionwithstacktrace.cc"
)

set(SOURCES ${SOURCE_FILES} ${GAME_SOURCES})

add_executable(performous_test ${SOURCES})
add_link_options(-rdynamic)

find_package(Boost 1.55 REQUIRED COMPONENTS program_options iostreams system locale)
target_link_libraries(performous_test PRIVATE ${Boost_LIBRARIES})

find_package(fmt REQUIRED CONFIG)
target_link_libraries(performous_test PRIVATE fmt::fmt)

target_link_libraries(performous_test PRIVATE GTest::GTest GTest::Main)

target_include_directories(performous_test PRIVATE ..)

gtest_discover_tests(performous_test)
