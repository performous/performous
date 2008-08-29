project(libda)
cmake_minimum_required(VERSION 2.6)

find_package(Alsa)
find_package(GStreamer)
find_package(Portaudio)
find_package(PulseAudio)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.34)

set(SOURCES audio.cpp)

if (ALSA_FOUND)
	include_directories(${ALSA_INCLUDE_DIR})
	include_directories(${CMAKE_SOURCE_DIR})
	set(LIBS ${LIBS} ${ALSA_LIBRARIES})
	set(SOURCES ${SOURCES} audio_dev_alsa.cpp)
endif (ALSA_FOUND)

if (GSTREAMER_LIBRARIES)
	include_directories(${GSTREAMER_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${GSTREAMER_LIBRARIES})
	set(SOURCES ${SOURCES} audio_dev_gst.cpp)
endif (GSTREAMER_LIBRARIES)

if (PORTAUDIO_FOUND)
	include_directories(${PORTAUDIO_INCLUDE_DIRS})
	set(LIBS ${LIBS} ${PORTAUDIO_LIBRARIES})
	set(SOURCES ${SOURCES} audio_dev_pa18.cpp)
endif (PORTAUDIO_FOUND)

if (PULSEAUDIO_FOUND)
	include_directories(${PULSEAUDIO_INCLUDE_DIR})
	set(LIBS ${LIBS} ${PULSEAUDIO_LIBRARIES})
	set(SOURCES ${SOURCES} audio_dev_pulse.cpp)
endif (PULSEAUDIO_FOUND)

if (Boost_FOUND)
	include_directories(${BOOST_INCLUDE_DIR})
	set(LIBS ${LIBS} ${BOOST_LIBRARIES})
	set(SOURCES ${SOURCES} audio_dev_tone.cpp)
endif (Boost_FOUND)

add_library(da ${SOURCES})
target_link_libraries(da ${LIBS})

install(TARGETS da DESTINATION lib)

