set(LibDAConf ${CMAKE_CURRENT_BINARY_DIR}/LibDAConfig.cmake)
set(LibDAConfVer ${CMAKE_CURRENT_BINARY_DIR}/LibDAConfigVersion.cmake)
configure_file(LibDAConfig.cmake.in ${LibDAConf} ESCAPE_QUOTES @ONLY)
configure_file(LibDAConfigVersion.cmake.in ${LibDAConfVer} ESCAPE_QUOTES @ONLY)
install(EXPORT LibDA DESTINATION ${LibDA_CMAKE_DIR})
install(FILES
	${LibDAConf}
	${LibDAConfVer}
	DESTINATION ${LibDA_CMAKE_DIR}
)

