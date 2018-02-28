cmake_minimum_required(VERSION 2.8)

# Libraries

find_package(Boost 1.34 REQUIRED COMPONENTS filesystem program_options system)
include_directories(${Boost_INCLUDE_DIRS})

# Find all the libs that don't require extra parameters

foreach(lib LibXML++ ZLIB JPEG PNG ZLIB)
find_package(${lib})
	if (${lib}_FOUND)
		include_directories(${${lib}_INCLUDE_DIRS})
		add_definitions(${${lib}_DEFINITIONS})
	endif (${lib}_FOUND)
endforeach(lib)

if (ZLIB_FOUND)
	if (LibXML++_FOUND)
		add_executable(ss_extract ss_extract.cc pak.cc ipu_conv.cc ss_cover.cc ../common/image.cc)
		target_link_libraries(ss_extract ${LibXML++_LIBRARIES} ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES})
		set(targets ${targets} ss_extract)

		add_executable(ss_cover_conv cover_conv.cc pak.cc ss_cover.cc ../common/image.cc)
		target_link_libraries(ss_cover_conv ${LibXML++_LIBRARIES} ${Boost_LIBRARIES} ${ZLIB_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES})
		set(targets ${targets} ss_cover_conv)
	else (LibXML++_FOUND)
		message("No LibXML++ found, not building ss_extract nor ss_cover_conv")
	endif (LibXML++_FOUND)

	add_executable(ss_pak_extract pak_extract.cc pak.cc)
	target_link_libraries(ss_pak_extract ${Boost_LIBRARIES} ${ZLIB_LIBRARIES})
	set(targets ${targets} ss_pak_extract)

	add_executable(itg_pck itg_pck.cc)
	target_link_libraries(itg_pck ${Boost_LIBRARIES} ${ZLIB_LIBRARIES})
	set(targets ${targets} itg_pck)

	add_executable(ss_chc_decode ss_chc_decode.cc)
	target_link_libraries(ss_chc_decode ${Boost_LIBRARIES} ${ZLIB_LIBRARIES})
	set(targets ${targets} ss_chc_decode)

	add_executable(ss_adpcm_decode adpcm_decode.cc pak.cc)
	target_link_libraries(ss_adpcm_decode ${ZLIB_LIBRARIES})
	set(targets ${targets} ss_adpcm_decode)
endif()

add_executable(ss_archive_extract archive_extract.cc)
target_link_libraries(ss_archive_extract ${Boost_LIBRARIES})
set(targets ${targets} ss_archive_extract)

add_executable(gh_fsb_decrypt gh_fsb/fsbext.c)
add_executable(gh_xen_decrypt gh_xen_decrypt.cc)
add_executable(ss_ipu_conv ipu_conv.cc ipuconvmain.cc pak.cc)
set(targets ${targets} gh_fsb_decrypt gh_xen_decrypt ss_adpcm_decode ss_ipu_conv)

# add install target:
if(WIN32)
	install(TARGETS ${targets} DESTINATION tools)
else()
	install(TARGETS ${targets} DESTINATION bin)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.hh" "${CMAKE_CURRENT_BINARY_DIR}/config.hh" @ONLY)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
