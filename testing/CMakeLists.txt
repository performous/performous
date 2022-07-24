cmake_minimum_required(VERSION 3.15)

set(SOURCE_FILES 
    "colortest.cc"
    "configitemtest.cc"
	"ffttest.cc"
    "fixednotegraphscalertest.cc"
    "notegraphscalerfactorytest.cc"
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
	"../game/guitarchords/fft.cc"
)

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES} ${GAME_SOURCES})

add_executable(performous_test ${SOURCES})

find_package(Boost 1.55 REQUIRED COMPONENTS program_options iostreams system locale)
target_link_libraries(performous_test PRIVATE ${Boost_LIBRARIES})

find_package(fmt REQUIRED CONFIG)
target_link_libraries(performous_test PRIVATE fmt::fmt)

set(Aubio_REQUIRED_VERSION "0.4.9")

find_package(Aubio ${Aubio_REQUIRED_VERSION} REQUIRED)
target_include_directories(performous_test SYSTEM PRIVATE ${Aubio_INCLUDE_DIRS})
target_link_libraries(performous_test PRIVATE ${Aubio_LIBRARIES})
add_definitions(${Aubio_DEFINITIONS})

target_link_libraries(performous_test PRIVATE GTest::GTest gmock GTest::Main aubio)

target_include_directories(performous_test PRIVATE ".." "../game")

gtest_discover_tests(performous_test)
