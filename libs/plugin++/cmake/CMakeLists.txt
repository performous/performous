set(Plugin++Conf ${CMAKE_CURRENT_BINARY_DIR}/Plugin++Config.cmake)
set(Plugin++ConfVer ${CMAKE_CURRENT_BINARY_DIR}/Plugin++ConfigVersion.cmake)
configure_file(Plugin++Config.cmake.in ${Plugin++Conf} ESCAPE_QUOTES @ONLY)
configure_file(Plugin++ConfigVersion.cmake.in ${Plugin++ConfVer} ESCAPE_QUOTES @ONLY)
install(EXPORT Plugin++ DESTINATION ${Plugin++_CMAKE_DIR})
install(FILES
	${Plugin++Conf}
	${Plugin++ConfVer}
	DESTINATION ${Plugin++_CMAKE_DIR}
)

