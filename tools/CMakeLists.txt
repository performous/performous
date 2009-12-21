cmake_minimum_required(VERSION 2.6)

# Libraries

find_package(Boost 1.34 REQUIRED COMPONENTS filesystem program_options)
include_directories(${Boost_INCLUDE_DIRS})

# Find all the libs that don't require extra parameters
foreach(lib LibXML++ Magick++ Z Jpeg Tiff Png Freetype Z)
	find_package(${lib})
	if (${lib}_FOUND)
		include_directories(${${lib}_INCLUDE_DIRS})
		add_definitions(${${lib}_DEFINITIONS})
	endif (${lib}_FOUND)
endforeach(lib)

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "GCC detected, enabling warnings")
	# Magick++ on OSX uses long long and thus cannot take -pedantic
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -Wextra")
endif(CMAKE_COMPILER_IS_GNUCXX)

if (LibXML++_FOUND)
	if (Boost_FOUND)
		if (Magick++_FOUND)
			add_executable(ss_extract ss_extract.cpp pak.cpp ipu_conv.cpp ss_cover.cpp)
			target_link_libraries(ss_extract ${LibXML++_LIBRARIES} ${Boost_LIBRARIES} ${Magick++_LIBRARIES} ${Z_LIBRARIES} ${Jpeg_LIBRARIES} ${Tiff_LIBRARIES} ${Png_LIBRARIES} ${Freetype_LIBRARY} ${Magick++_LIBRARIES})
			set(targets ${targets} ss_extract)
		else (Magick++_FOUND)
			message("No Magick++ found, not building ss_extract")
		endif (Magick++_FOUND)
	else (Boost_FOUND)
		message("No Boost.Filesystem found, not building ss_extract")
	endif (Boost_FOUND)

	if (Magick++_FOUND)
		add_executable(ss_cover_conv cover_conv.cpp pak.cpp ss_cover.cpp)
		target_link_libraries(ss_cover_conv ${Magick++_LIBRARIES} ${LibXML++_LIBRARIES} ${Z_LIBRARIES} ${Jpeg_LIBRARIES} ${Tiff_LIBRARIES} ${Png_LIBRARIES} ${Freetype_LIBRARY} ${Magick++_LIBRARIES})
		set(targets ${targets} ss_cover_conv)
	else (Magick++_FOUND)
		message("No Magick++ found, not building ss_cover_conv")
	endif (Magick++_FOUND)
else (LibXML++_FOUND)
	message("No LibXML++ found, not building ss_extract nor ss_cover_conv")
endif (LibXML++_FOUND)

if (Boost_FOUND)
	add_executable(ss_pak_extract pak_extract.cpp pak.cpp)
	target_link_libraries(ss_pak_extract ${Boost_LIBRARIES})
	if (Z_FOUND)
		add_executable(itg_pck itg_pck.cc)
		target_link_libraries(itg_pck ${Boost_LIBRARIES} ${Z_LIBRARIES})
	endif()
endif (Boost_FOUND)

add_executable(ss_adpcm_decode adpcm_decode.cpp pak.cpp)
add_executable(ss_ipu_decode ipu_decode.cpp)
add_executable(ss_ipu_conv ipu_conv.cpp ipuconvmain.cpp pak.cpp)
set(targets ${targets} ss_pak_extract ss_adpcm_decode ss_ipu_decode ss_ipu_conv itg_pck)

# add install target:
install(TARGETS ${targets} DESTINATION bin)

