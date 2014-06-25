set(APPLICATION_FILE "performous.desktop")
set(PIXMAP_FILE      "themes/default/icon.svg")

# Install launcher and fonts on system level
if(APPLE)
	install(FILES ${APPLICATION_FILE} DESTINATION "Resources/applications/")
	install(FILES ${PIXMAP_FILE} DESTINATION "Resources/pixmaps/" RENAME "performous.svg")
	#file(GLOB FONT_FILES "fonts/*")
	#install(FILES ${FONT_FILES} DESTINATION "Resources/fonts/truetype/performous")
elseif(UNIX)
	install(FILES ${APPLICATION_FILE} DESTINATION "share/applications/")
	install(FILES ${PIXMAP_FILE} DESTINATION "share/pixmaps/" RENAME "performous.svg")
	#file(GLOB FONT_FILES "fonts/*")
	#install(FILES ${FONT_FILES} DESTINATION "share/fonts/truetype/performous")
endif()

install(DIRECTORY backgrounds config fonts shaders sounds themes xsl DESTINATION ${SHARE_INSTALL})

