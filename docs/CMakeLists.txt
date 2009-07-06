
if(UNIX)
	find_program(HELP2MAN help2man DOC "Location of the help2man program")
	find_program(GZIP gzip DOC "Location of the gzip program")
	mark_as_advanced(HELP2MAN)
	mark_as_advanced(GZIP)
	if(HELP2MAN AND GZIP)
		set(MANFILE ${CMAKE_CURRENT_BINARY_DIR}/performous.1.gz)
		set(H2MFILE ${CMAKE_CURRENT_SOURCE_DIR}/performous.h2m)
		set(PERFORMOUS_EXEC ${CMAKE_BINARY_DIR}/game/performous)
		add_custom_command(
			OUTPUT ${MANFILE}
			COMMAND ${HELP2MAN} ${PERFORMOUS_EXEC} -i ${H2MFILE} -N | ${GZIP} > ${MANFILE}
			MAIN_DEPENDENCY ${H2MFILE}
			DEPENDENCY ${PERFORMOUS_EXEC}
			COMMENT "Building Performous man page"
			VERBATIM
		)
		add_custom_target(manpage ALL DEPENDS ${MANFILE})
		install(FILES ${MANFILE} DESTINATION share/man/man1)
	else(HELP2MAN AND GZIP)
		message("WARNING: Can't locate help2man or gzip; man pages will not be generated")
	endif(HELP2MAN AND GZIP)
endif(UNIX)


