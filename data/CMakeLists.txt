set(APPLICATION_FILE "performous.desktop")
set(PIXMAP_FILE      "performous.xpm")

if(UNIX)
	install(FILES "${APPLICATION_FILE}" DESTINATION "share/applications/")
	install(FILES "${PIXMAP_FILE}" DESTINATION "share/pixmaps")
endif(UNIX)
