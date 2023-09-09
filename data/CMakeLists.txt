set(APPLICATION_FILE "performous.desktop")
set(PIXMAP_FILE      "themes/default/icon.svg")
set(METAINFO_FILE    "performous.metainfo.xml")

# Install launcher and fonts on system level
if(UNIX AND NOT APPLE)
	install(FILES "${APPLICATION_FILE}" DESTINATION "/usr/share/applications/")
	install(FILES "${PIXMAP_FILE}" DESTINATION "/usr/share/pixmaps/" RENAME "performous.svg")

    # Translate the MetaInfo XML file (TODO: just copy the file if not Gettext_FOUND)
	add_custom_command(
		OUTPUT ${METAINFO_FILE}
		COMMAND ${ITSTOOL_EXECUTABLE} -j ${CMAKE_CURRENT_SOURCE_DIR}/${METAINFO_FILE}.in -o ${CMAKE_CURRENT_BINARY_DIR}/${METAINFO_FILE} ${_mofiles}
		DEPENDS translations
		COMMENT "Building translated MetaInfo file"
		VERBATIM)
	# Install it
	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${METAINFO_FILE} DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo/")
	add_custom_target(metainfo ALL DEPENDS ${METAINFO_FILE})
endif()

install(DIRECTORY backgrounds config fonts shaders sounds themes xsl DESTINATION ${SHARE_INSTALL})

