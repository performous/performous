set(APPLICATION_FILE "performous.desktop")
set(PIXMAP_FILE      "performous.xpm")

install( FILES "${APPLICATION_FILE}" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications/")
install( FILES "${PIXMAP_FILE}" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pixmaps")

