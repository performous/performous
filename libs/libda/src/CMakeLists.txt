cmake_minimum_required(VERSION 2.6)

add_library(da SHARED audio.cpp)
target_link_libraries(da ${Boost_LIBRARIES} ${Plugin++_LIBRARIES})

set_target_properties(da PROPERTIES
	INSTALL_NAME_DIR @rpath
	VERSION "${LibDA_VERSION}"
	SOVERSION "${LibDA_VERSION_MAJOR}"
	DEBUG_POSTFIX -d
	INSTALL_RPATH_USE_LINK_PATH TRUE
)

install(TARGETS da EXPORT LibDA DESTINATION lib${LIB_SUFFIX})

