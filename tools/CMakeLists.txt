PROJECT(USNG)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/Modules/)

SET(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost COMPONENTS filesystem)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(MAGICK ImageMagick++)
PKG_CHECK_MODULES(LIBXMLPP libxml++-2.6)
PKG_CHECK_MODULES(PORTAUDIO portaudio-1.0)

IF (LIBXMLPP_FOUND)
    INCLUDE_DIRECTORIES(${LIBXMLPP_INCLUDE_DIRS})

	IF (Boost_FOUND)
		ADD_EXECUTABLE(ss_extract ss_extract.cpp pak.cpp ipu_conv.cpp)
		TARGET_LINK_LIBRARIES(ss_extract ${LIBXMLPP_LIBRARIES} ${Boost_LIBRARIES})
		SET(targets ${targets} ss_extract)
	ELSE (Boost_FOUND)
		MESSAGE("No Boost Filesystem found, not building ss_extract")
	ENDIF (Boost_FOUND)

    IF (MAGICK_FOUND)
        INCLUDE_DIRECTORIES(${MAGICK_INCLUDE_DIRS})
        ADD_EXECUTABLE(cover_conv cover_conv.cpp pak.cpp)
        TARGET_LINK_LIBRARIES(cover_conv ${MAGICK_LIBRARIES} ${LIBXMLPP_LIBRARIES})
		SET(targets ${targets} cover_conv)
    ELSE (MAGICK_FOUND)
        MESSAGE("No Magick++ found, not building cover_conv")
    ENDIF (MAGICK_FOUND)
ELSE (LIBXMLPP_FOUND)
    MESSAGE("No libxml++ found, not building ss_extract nor cover_conv")
ENDIF (LIBXMLPP_FOUND)

ADD_EXECUTABLE(pak_extract pak_extract.cpp pak.cpp)
ADD_EXECUTABLE(adpcm_decode adpcm_decode.cpp pak.cpp)
ADD_EXECUTABLE(ipu_decode ipu_decode.cpp)
ADD_EXECUTABLE(ipu_conv ipu_conv.cpp ipuconvmain.cpp pak.cpp)
SET(targets ${targets} pak_extract adpcm_decode ipu_decode ipu_conv)

IF (PORTAUDIO_FOUND)
    INCLUDE_DIRECTORIES(${PORTAUDIO_INCLUDE_DIRS})
    ADD_EXECUTABLE(adpcm_play adpcm_play.cpp pak.cpp)
    SET(targets ${targets} adpcm_play)
    TARGET_LINK_LIBRARIES(adpcm_play ${PORTAUDIO_LIBRARIES})
ELSE (PORTAUDIO_FOUND)
    MESSAGE("No PortAudio v18 found, not building adpcm_play")
ENDIF (PORTAUDIO_FOUND)


# add install target:
INSTALL(TARGETS ${targets} DESTINATION bin)

