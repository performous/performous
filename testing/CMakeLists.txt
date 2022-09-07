cmake_minimum_required(VERSION 3.15)

set(SOURCE_FILES
	"analyzertest.cc"
	"colortest.cc"
	"configitemtest.cc"
	"fixednotegraphscalertest.cc"
	"notegraphscalerfactorytest.cc"
	"printer.cc"
	"ringbuffertest.cc"
	"utiltest.cc"
)
set(GAME_SOURCES
	"../game/analyzer.cc"
	"../game/color.cc"
	"../game/configitem.cc"
	"../game/dynamicnotegraphscaler.cc"
	"../game/fixednotegraphscaler.cc"
	"../game/musicalscale.cc"
	"../game/notes.cc"
	"../game/notegraphscalerfactory.cc"
	"../game/tone.cc"
)

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES} ${GAME_SOURCES})

add_executable(performous_test ${SOURCES})

find_package(Boost 1.55 REQUIRED COMPONENTS program_options iostreams system locale)
target_link_libraries(performous_test PRIVATE ${Boost_LIBRARIES})

find_package(fmt REQUIRED CONFIG)
target_link_libraries(performous_test PRIVATE fmt::fmt)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.23)
	target_link_libraries(performous_test PRIVATE GTest::gtest GTest::gtest_main GTest::gmock)
elseif(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.20)
	target_link_libraries(performous_test PRIVATE GTest::gtest GTest::gtest_main)
else()
	target_link_libraries(performous_test PRIVATE GTest::GTest GTest::Main)
endif()

target_include_directories(performous_test PRIVATE ".." "../game")

gtest_discover_tests(performous_test)
