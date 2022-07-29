cmake_minimum_required(VERSION 3.15)

set(SOURCE_FILES
	"chordtest.cc"
	"chorddetectortest.cc"
	"colortest.cc"
	"configitemtest.cc"
	"ffttest.cc"
	"fftsampletest.cc"
	"fixednotegraphscalertest.cc"
	"notegraphscalerfactorytest.cc"
	"stringtest.cc"
	"utiltest.cc"
	"common.cc"
)
set(GAME_SOURCES
	"../game/color.cc"
	"../game/configitem.cc"
	"../game/dynamicnotegraphscaler.cc"
	"../game/fixednotegraphscaler.cc"
	"../game/musicalscale.cc"
	"../game/notes.cc"
	"../game/notegraphscalerfactory.cc"
	"../game/guitarchords/chord.cc"
	"../game/guitarchords/chorddetector.cc"
	"../game/guitarchords/chordmatcher.cc"
	"../game/guitarchords/fft.cc"
	"../game/guitarchords/profile.cc"
	"../game/guitarchords/profilecreator.cc"
	"../game/guitarchords/string.cc"
)

set(SOURCES ${SOURCE_FILES} ${HEADER_FILES} ${GAME_SOURCES})

add_executable(performous_test ${SOURCES})

find_package(Boost 1.55 REQUIRED COMPONENTS program_options iostreams system locale)
target_link_libraries(performous_test PRIVATE ${Boost_LIBRARIES})

find_package(fmt REQUIRED CONFIG)
target_link_libraries(performous_test PRIVATE fmt::fmt)

set(Aubio_REQUIRED_VERSION "0.4.9")

#foreach(lib ${OUR_LIBS} SDL2 PangoCairo LibRSVG LibXML++ AVFormat SWResample SWScale ZLIB JPEG PNG PortAudio Fontconfig GLM Json Ced Aubio)
foreach(lib ${OUR_LIBS} Aubio)
	find_package(${lib} ${${lib}_REQUIRED_VERSION} REQUIRED)
	message(STATUS "${lib} includes: ${${lib}_INCLUDE_DIRS}")
	target_include_directories(performous SYSTEM PRIVATE ${${lib}_INCLUDE_DIRS})
	target_link_libraries(performous PRIVATE ${${lib}_LIBRARIES})
	add_definitions(${${lib}_DEFINITIONS})
endforeach(lib)

target_link_libraries(performous_test PRIVATE GTest::GTest gmock GTest::Main aubio)

target_include_directories(performous_test PRIVATE "." ".." "../game")

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

gtest_discover_tests(performous_test)
