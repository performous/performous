cmake_minimum_required(VERSION 2.6)

# Libraries

find_package(Boost 1.34 REQUIRED COMPONENTS filesystem program_options system)
include_directories(${Boost_INCLUDE_DIRS})

# Find all the libs that don't require extra parameters

if (APPLE)
	foreach(lib LibXML++ Z Jpeg Tiff Png Freetype Z)
	find_package(${lib})
		if (${lib}_FOUND)
		include_directories(${${lib}_INCLUDE_DIRS})
			add_definitions(${${lib}_DEFINITIONS})
		endif (${lib}_FOUND)
	endforeach(lib)

# CMake cannot properly detect Magick++ on Mac the other way.

	find_package(ImageMagick COMPONENTS Magick++)
	include_directories(${ImageMagick_INCLUDE_DIRS})
else (APPLE)
	foreach(lib LibXML++ Z Jpeg Tiff Png Freetype Z)
		find_package(${lib})
		if (${lib}_FOUND)
			include_directories(${${lib}_INCLUDE_DIRS})
			add_definitions(${${lib}_DEFINITIONS})
		endif (${lib}_FOUND)
	endforeach(lib)

	find_package(ImageMagick COMPONENTS Magick++)
	find_package(PkgConfig)
	PKG_CHECK_MODULES(IMAGEMAGICK Magick++ MagickWand MagickCore)
	include_directories(${ImageMagick_INCLUDE_DIRS})
endif (APPLE)

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, enabling warnings")
	# Magick++ on OSX uses long long and thus cannot take -pedantic
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -Wextra")
endif(CMAKE_COMPILER_IS_GNUCXX)

if (LibXML++_FOUND)
	if (Boost_FOUND)
		if (ImageMagick_FOUND OR Magick++_FOUND) 
			if (Z_FOUND)
				add_executable(ss_extract ss_extract.cc pak.cc ipu_conv.cc ss_cover.cc)
				target_link_libraries(ss_extract ${LibXML++_LIBRARIES} ${Boost_LIBRARIES} ${Magick++_LIBRARIES} ${Z_LIBRARIES} ${Jpeg_LIBRARIES} ${Tiff_LIBRARIES} ${Png_LIBRARIES} ${Freetype_LIBRARY} ${ImageMagick_LIBRARIES})
				set(targets ${targets} ss_extract)
			else (Z_FOUND)
				message("No zlib found, not building ss_extract")
			endif (Z_FOUND)
		else (ImageMagick_FOUND OR Magick++_FOUND)
			message("No Magick++ found, not building ss_extract")
		endif (ImageMagick_FOUND OR Magick++_FOUND)
	else (Boost_FOUND)
		message("No Boost.Filesystem found, not building ss_extract")
	endif (Boost_FOUND)

	if (ImageMagick_FOUND OR Magick++_FOUND)
		add_executable(ss_cover_conv cover_conv.cc pak.cc ss_cover.cc)
		target_link_libraries(ss_cover_conv ${ImageMagick_LIBRARIES} ${LibXML++_LIBRARIES} ${Z_LIBRARIES} ${Jpeg_LIBRARIES} ${Tiff_LIBRARIES} ${Png_LIBRARIES} ${Freetype_LIBRARY} ${ImageMagick_LIBRARIES} ${Magick++_LIBRARIES})
		set(targets ${targets} ss_cover_conv)
	else (ImageMagick_FOUND OR Magick++_FOUND)
		message("No Magick++ found, not building ss_cover_conv")
	endif (ImageMagick_FOUND OR Magick++_FOUND)
else (LibXML++_FOUND)
	message("No LibXML++ found, not building ss_extract nor ss_cover_conv")
endif (LibXML++_FOUND)

if (Boost_FOUND AND Z_FOUND)
	add_executable(ss_archive_extract archive_extract.cc)
	target_link_libraries(ss_archive_extract ${Boost_LIBRARIES} ${Z_LIBRARIES})
	set(targets ${targets} ss_archive_extract)

	add_executable(ss_pak_extract pak_extract.cc pak.cc)
	target_link_libraries(ss_pak_extract ${Boost_LIBRARIES} ${Z_LIBRARIES})
	set(targets ${targets} ss_pak_extract)

	add_executable(itg_pck itg_pck.cc)
	target_link_libraries(itg_pck ${Boost_LIBRARIES} ${Z_LIBRARIES})
	set(targets ${targets} itg_pck)

	add_executable(ss_chc_decode ss_chc_decode.cc)
	target_link_libraries(ss_chc_decode ${Boost_LIBRARIES} ${Z_LIBRARIES})
	set(targets ${targets} ss_chc_decode)

endif()

if (Z_FOUND)
	add_executable(ss_adpcm_decode adpcm_decode.cc pak.cc)
	target_link_libraries(ss_adpcm_decode ${Z_LIBRARIES})
	set(targets ${targets} ss_adpcm_decode)
endif()

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

