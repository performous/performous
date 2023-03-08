cmake_minimum_required(VERSION 3.15)
include(GoogleTest)

set(SOURCE_FILES
	"analyzertest.cc"
	"colortest.cc"
	"configitemtest.cc"
	"cycletest.cc"
	"fixednotegraphscalertest.cc"
	"microphones_test.cc"
	"notegraphscalerfactorytest.cc"
	"ringbuffertest.cc"
	"songfiltertest.cc"
	"songcollectionfiltertest.cc"
	"utiltest.cc"

	"common.cc"
	"main.cc"
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
	"../game/log.cc"
	"../game/i18n.cc"
	"../game/json.cc"
	"../game/microphones.cc"
	"../game/midifile.cc"
	"../game/musicalscale.cc"
	"../game/notes.cc"
	"../game/notegraphscalerfactory.cc"
	"../game/platform.cc"
	"../game/song.cc"
	"../game/songcollectionfilter.cc"
	"../game/songfilter.cc"
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
		target_link_libraries(performous_test PRIVATE GTest::gtest GTest::gmock)
	else()
		if(NOT TARGET GTest::gmock)
			find_package(GMock MODULE REQUIRED)
			add_library(GTest::gmock ALIAS GMock)
		endif()
		target_link_libraries(performous_test PRIVATE GTest::GTest GTest::gmock)
	endif()

	target_include_directories(performous_test PRIVATE ".." "../game" "${Performous_BINARY_DIR}/game")

	set(Aubio_REQUIRED_VERSION "0.4.9")

	#foreach(lib ${OUR_LIBS} SDL2 PangoCairo LibRSVG LibXML++ AVFormat SWResample SWScale ZLIB JPEG PNG PortAudio Fontconfig GLM Json Ced Aubio)
    foreach(lib ${OUR_LIBS} PangoCairo LibXML++ AVFormat Ced Aubio)
		find_package(${lib} ${${lib}_REQUIRED_VERSION} REQUIRED)
		message(STATUS "${lib} includes: ${${lib}_INCLUDE_DIRS}")
		target_include_directories(performous_test SYSTEM PRIVATE ${${lib}_INCLUDE_DIRS})
		target_link_libraries(performous_test PRIVATE ${${lib}_LIBRARIES})
		add_definitions(${${lib}_DEFINITIONS})
	endforeach(lib)

    find_package(SDL2 ${SDL2_REQUIRED_VERSION} REQUIRED)
    target_include_directories(performous_test SYSTEM PRIVATE ${SDL2_INCLUDE_DIRS})
    #target_link_libraries(performous_test PRIVATE ${SDL2_LIBRARIES} SDL2main)
add_definitions(-DSDL_MAIN_HANDLED)

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
	
	find_package(SDL2 REQUIRED)
	if (TARGET SDL2::SDL2)
		target_link_libraries(performous_test PRIVATE SDL2::SDL2)
	else()
		list(REMOVE_ITEM SDL2_LIBRARIES SDL2::SDL2main)
		target_link_libraries(performous_test PRIVATE ${SDL2_LIBRARIES})
	endif()
	target_include_directories(performous_test PRIVATE ${SDL2_INCLUDE_DIRS}) # Newer SDL versions have proper CMake targets making this unnecessary, but we can't take it for granted.

	if(APPLE)
	target_link_libraries(performous_test PRIVATE "-framework CoreFoundation")
	endif()

	gtest_discover_tests(performous_test PROPERTIES TEST_DISCOVERY_TIMEOUT 30)

else()
	message(STATUS "Testing disabled: Package gtest missing")
endif()
