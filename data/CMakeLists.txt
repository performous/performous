set(APPLICATION_FILE "performous.desktop")
set(PIXMAP_FILE      "themes/default/icon.svg")

# Install launcher and fonts on system level
if(UNIX AND NOT APPLE)
	install(FILES "${APPLICATION_FILE}" DESTINATION "/usr/share/applications/")
	install(FILES "${PIXMAP_FILE}" DESTINATION "/usr/share/pixmaps/" RENAME "performous.svg")
endif()

install(DIRECTORY backgrounds config fonts shaders sounds themes xsl DESTINATION ${SHARE_INSTALL})

