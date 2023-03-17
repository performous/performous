cmake_minimum_required(VERSION 3.15)
include(GoogleTest)

set(SOURCE_FILES
	"analyzertest.cc"
	"colortest.cc"
	"configitemtest.cc"
	"cycletest.cc"
	"fixednotegraphscalertest.cc"
	"notegraphscalerfactorytest.cc"
	"ringbuffertest.cc"
	"songfiltertest.cc"
	"songcollectionfiltertest.cc"
	"utiltest.cc"

	"common.cc"
	"printer.cc"
)
set(GAME_SOURCES
	"../game/analyzer.cc"
	"../game/color.cc"
	"../game/configitem.cc"
#	"../game/configuration.cc"
	"../game/dynamicnotegraphscaler.cc"
	"../game/execname.cc"
	"../game/fixednotegraphscaler.cc"
	"../game/fs.cc"
	"../game/i18n.cc"
	"../game/midifile.cc"
	"../game/musicalscale.cc"
	"../game/notes.cc"
	"../game/notegraphscalerfactory.cc"
	"../game/platform.cc"
	"../game/song.cc"
	"../game/songcollectionfilter.cc"
	"../game/songfilter.cc"
	"../game/songparser.cc"
	"../game/songparserexception.cc"
	"../game/songparser-ini.cc"
	"../game/songparser-mid.cc"
	"../game/songparser-sm.cc"
	"../game/songparser-txt.cc"
	"../game/songparser-xml.cc"
	"../game/tone.cc"
	"../game/unicode.cc"
	"../game/util.cc"
)

set(GTEST_REQUIRED "")

if(BUILD_TESTS STREQUAL "ON")
	set(GTEST_REQUIRED "REQUIRED")
endif()

find_package(GTest ${GTEST_REQUIRED})

if(GTest_FOUND)
	message(STATUS "Testing enabled: Building test host")

	set(SOURCES ${SOURCE_FILES} ${HEADER_FILES} ${GAME_SOURCES})

	add_executable(performous_test ${SOURCES})

	find_package(Boost 1.55 REQUIRED COMPONENTS program_options iostreams system locale)
	target_link_libraries(performous_test PRIVATE ${Boost_LIBRARIES})

	find_package(fmt REQUIRED CONFIG)
	target_link_libraries(performous_test PRIVATE fmt::fmt)

	if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.20)
		target_link_libraries(performous_test PRIVATE GTest::gtest GTest::gtest_main GTest::gmock)
	else()
		if(NOT TARGET GTest::gmock)
			find_package(GMock MODULE REQUIRED)
			add_library(GTest::gmock ALIAS GMock)
		endif()
		target_link_libraries(performous_test PRIVATE GTest::GTest GTest::Main GTest::gmock)
	endif()

	target_include_directories(performous_test PRIVATE ".." "../game")

	set(Aubio_REQUIRED_VERSION "0.4.9")

	#foreach(lib ${OUR_LIBS} SDL2 PangoCairo LibRSVG LibXML++ AVFormat SWResample SWScale ZLIB JPEG PNG PortAudio Fontconfig GLM Json Ced Aubio)
	foreach(lib ${OUR_LIBS} SDL2 PangoCairo LibXML++ AVFormat Ced Aubio)
		find_package(${lib} ${${lib}_REQUIRED_VERSION} REQUIRED)
		message(STATUS "${lib} includes: ${${lib}_INCLUDE_DIRS}")
		target_include_directories(performous_test SYSTEM PRIVATE ${${lib}_INCLUDE_DIRS})
		target_link_libraries(performous_test PRIVATE ${${lib}_LIBRARIES})
		add_definitions(${${lib}_DEFINITIONS})
	endforeach(lib)

	find_package(ICU 60 REQUIRED uc data i18n io)
	target_include_directories(performous_test SYSTEM PRIVATE ${ICU_INCLUDE_DIRS})
	target_link_libraries(performous_test PRIVATE ${ICU_LIBRARIES})

# Generate config.hh
configure_file("../game/config.cmake.hh" "${CMAKE_CURRENT_BINARY_DIR}/config.hh" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")


	target_include_directories(performous_test PRIVATE "." ".." "../game" "../3rdparty/ced")
	target_link_libraries(performous_test PRIVATE fs ced aubio)

	if(WIN32 AND MSVC)
#		target_compile_options(performous_test PUBLIC /WX)
	else()
		target_compile_options(performous_test PUBLIC -Werror)
	endif()

	gtest_discover_tests(performous_test)

else()
	message(STATUS "Testing disabled: Package gtest missing")
endif()
