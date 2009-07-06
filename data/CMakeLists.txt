set(APPLICATION_FILE "performous.desktop")
set(PIXMAP_FILE      "performous.xpm")
set(CONFIG_FILE      "performous.xml")

if(UNIX)
	install(FILES ${APPLICATION_FILE} DESTINATION "share/applications/")
	install(FILES ${PIXMAP_FILE} DESTINATION "share/pixmaps")
	install(FILES ${PIXMAP_FILE} DESTINATION "share/pixmaps")
endif(UNIX)

install(FILES ${CONFIG_FILE} DESTINATION "${CMAKE_INSTALL_PREFIX}/${SHARE_INSTALL}/config/")

file(GLOB XSL_FILES "*.xsl")
install(FILES ${XSL_FILES} DESTINATION "${CMAKE_INSTALL_PREFIX}/${SHARE_INSTALL}/xsl/")

