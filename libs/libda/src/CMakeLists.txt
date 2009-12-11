cmake_minimum_required(VERSION 2.6)

add_library(da STATIC audio.cpp)
target_link_libraries(da ${Boost_LIBRARIES} ${Plugin++_LIBRARIES})

set_target_properties(da PROPERTIES
	INSTALL_NAME_DIR @rpath
	VERSION "${LibDA_VERSION}"
	SOVERSION "${LibDA_VERSION_MAJOR}"
	DEBUG_POSTFIX -d
	INSTALL_RPATH_USE_LINK_PATH TRUE
)

if(WIN32)
	find_package(Portaudio)
	if(Portaudio_FOUND)
		set(PLUGIN_PORTAUDIO_DESC "enabled, version ${Portaudio_VERSION}")
		set(PLUGIN_PORTAUDIO true)
	else(Portaudio_FOUND)
		message(FATAL_ERROR "Could not find Portaudio, it's required")
	endif(Portaudio_FOUND)
	
	include_directories(${Portaudio_INCLUDE_DIRS})
	include_directories(../plugins)
	target_link_libraries(da ${Portaudio_LIBRARIES})
endif(WIN32)

if(NOT SLAVE_LIB)
	install(TARGETS da EXPORT LibDA DESTINATION lib${LIB_SUFFIX})
endif()

