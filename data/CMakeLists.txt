set(APPLICATION_FILE "performous.desktop")
set(PIXMAP_FILE      "themes/default/icon.svg")

# Install launcher and fonts on system level
if(APPLE)
	install(FILES ${APPLICATION_FILE} DESTINATION "Resources/applications/" COMPONENT Application)
	install(FILES ${PIXMAP_FILE} DESTINATION "Resources/pixmaps/" RENAME "performous.svg" COMPONENT Application)
	#file(GLOB FONT_FILES "fonts/*")
	#install(FILES ${FONT_FILES} DESTINATION "Resources/fonts/truetype/performous" COMPONENT Application)
elseif(UNIX)
	install(FILES ${APPLICATION_FILE} DESTINATION "share/applications/" COMPONENT Application)
	install(FILES ${PIXMAP_FILE} DESTINATION "share/pixmaps/" RENAME "performous.svg" COMPONENT Application)
	#file(GLOB FONT_FILES "fonts/*")
	#install(FILES ${FONT_FILES} DESTINATION "share/fonts/truetype/performous" COMPONENT Application)
endif()

install(DIRECTORY backgrounds config fonts shaders sounds xsl DESTINATION ${SHARE_INSTALL} COMPONENT Application)
install(DIRECTORY themes DESTINATION ${SHARE_INSTALL} COMPONENT Themes)

