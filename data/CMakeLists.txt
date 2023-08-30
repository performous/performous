set(APPLICATION_FILE "performous.desktop")
set(PIXMAP_FILE      "themes/default/icon.svg")

# Install launcher and fonts on system level
if(UNIX AND NOT APPLE)
	install(FILES "${APPLICATION_FILE}" DESTINATION "${CMAKE_INSTALL_DATADIR}/applications/")
	install(FILES "${PIXMAP_FILE}" DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps/" RENAME "performous.svg")
endif()

install(DIRECTORY backgrounds config fonts shaders sounds themes xsl DESTINATION ${SHARE_INSTALL})

