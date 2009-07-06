
if(UNIX)
	find_program(HELP2MAN help2man DOC "Location of the help2man program")
	find_program(GZIP gzip DOC "Location of the gzip program")
	if(HELP2MAN AND GZIP)
		set(MANFILE ${CMAKE_CURRENT_BINARY_DIR}/performous.1.gz)
		add_custom_command(
			OUTPUT ${MANFILE}
			COMMAND ${HELP2MAN} performous -i ${CMAKE_CURRENT_SOURCE_DIR}/performous.h2m -N | ${GZIP} > ${MANFILE}
			MAIN_DEPENDENCY performous.h2m
			COMMENT "Building Performous man page"
			VERBATIM
		)
		add_custom_target(manpage ALL DEPENDS ${MANFILE})
		install(FILES ${MANFILE} DESTINATION share/man/man1)
	else(HELP2MAN AND GZIP)
		message("WARNING: Can't locate help2man or gzip; man pages will not be generated")
	endif(HELP2MAN AND GZIP)
endif(UNIX)


