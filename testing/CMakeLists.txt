cmake_minimum_required(VERSION 3.24)
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
	"utiltest.cc"
	"imagetypetest.cc"

	"main.cc"
	"printer.cc"
)
set(GAME_SOURCES
	"../game/analyzer.cc"
	"../game/color.cc"
	"../game/configitem.cc"
	"../game/dynamicnotegraphscaler.cc"
	"../game/execname.cc"
	"../game/fixednotegraphscaler.cc"
	"../game/fs.cc"
	"../game/image.cc"
	"../game/log.cc"
	"../game/microphones.cc"
	"../game/musicalscale.cc"
	"../game/notes.cc"
	"../game/notegraphscalerfactory.cc"
	"../game/platform.cc"
	"../game/tone.cc"
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

	find_package(fmt 8.1 REQUIRED CONFIG)
	target_link_libraries(performous_test PRIVATE fmt::fmt)

	# CppRESTSDK and their godforsaken U macro.
	if(fmt_VERSION VERSION_GREATER_EQUAL 9.0.0)
		target_compile_definitions(performous_test PRIVATE -D_TURN_OFF_PLATFORM_STRING)
	endif()

	find_package(ICU 65 REQUIRED uc data i18n io)
	target_link_libraries(performous_test PRIVATE ICU::uc ICU::data ICU::i18n ICU::io)


	find_package(Spdlog REQUIRED MODULE)

	target_link_libraries(performous_test PRIVATE GTest::gtest GTest::gtest_main GTest::gmock)

	target_compile_definitions(performous_test PRIVATE $<IF:$<NOT:$<BOOL:${SPDLOG_FMT_EXTERNAL_HO}>>,SPDLOG_FMT_EXTERNAL,SPDLOG_FMT_EXTERNAL_HO> FMT_USE_CONSTEXPR)
	target_link_libraries(performous_test PRIVATE $<IF:$<NOT:$<BOOL:${SPDLOG_FMT_EXTERNAL_HO}>>,fmt::fmt,fmt::fmt-header-only> spdlog::spdlog_header_only)

	target_include_directories(performous_test PRIVATE ".." "../game" "${Performous_BINARY_DIR}/game")

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

	# Image formats
	foreach(lib PangoCairo LibRSVG ZLIB JPEG PNG WebP)
		find_package(${lib} ${${lib}_REQUIRED_VERSION} REQUIRED)
		message(STATUS "${lib} includes: ${${lib}_INCLUDE_DIRS}")
		target_include_directories(performous_test SYSTEM PRIVATE ${${lib}_INCLUDE_DIRS})
		target_link_libraries(performous_test PRIVATE ${${lib}_LIBRARIES})
	endforeach(lib)


else()
	message(STATUS "Testing disabled: Package gtest missing")
endif()
