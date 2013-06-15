set(APPLICATION_FILE "performous.desktop")
set(PIXMAP_FILE      "../themes/default/icon.svg")
set(CONFIG_FILE      "schema.xml")
set(CONTROLLERS_FILE "controllers.xml")

file(GLOB FONT_FILES "fonts/*")

if(UNIX)
	install(FILES ${APPLICATION_FILE} DESTINATION "share/applications/")
	install(FILES ${PIXMAP_FILE} DESTINATION "share/pixmaps/" RENAME "performous.svg")
	install(FILES ${FONT_FILES} DESTINATION "share/fonts/truetype/performous")
else()
	install(FILES ${FONT_FILES} DESTINATION "${SHARE_INSTALL}/fonts/")
endif()

file(GLOB CONFIG_FILES "config/*")
install(FILES ${CONFIG_FILES} DESTINATION "${SHARE_INSTALL}/config/")

file(GLOB XSL_FILES "xsl/*")
install(FILES ${XSL_FILES} DESTINATION "${SHARE_INSTALL}/xsl/")

file(GLOB SOUND_FILES "sounds/*")
install(FILES ${SOUND_FILES} DESTINATION "${SHARE_INSTALL}/sounds/")

file(GLOB BG_FILES "backgrounds/*")
install(FILES ${BG_FILES} DESTINATION "${SHARE_INSTALL}/backgrounds/")

file(GLOB SHADER_FILES "shaders/*")
install(FILES ${SHADER_FILES} DESTINATION "${SHARE_INSTALL}/shaders/")

