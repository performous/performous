find_program(HELP2MAN help2man DOC "Location of the help2man program")
find_program(GZIP gzip DOC "Location of the gzip program")
mark_as_advanced(HELP2MAN)
mark_as_advanced(GZIP)
if(HELP2MAN AND GZIP)
	set(MANFILE ${CMAKE_CURRENT_BINARY_DIR}/performous.6.gz)
	set(H2MFILE ${CMAKE_CURRENT_SOURCE_DIR}/performous.h2m)
	add_custom_command(
		OUTPUT ${MANFILE}
		COMMAND ${HELP2MAN} "$<TARGET_FILE:performous>" -s 6 -i ${H2MFILE} -N | ${GZIP} > ${MANFILE}
		MAIN_DEPENDENCY ${H2MFILE}
		DEPENDS performous
		COMMENT "Building Performous man page"
		VERBATIM
	)
	add_custom_target(manpage ALL DEPENDS ${MANFILE})
	install(FILES ${MANFILE} TYPE MAN)
else(HELP2MAN AND GZIP)
	message("WARNING: One of the following is missing: help2man, gzip; performous man page will not be generated")
endif(HELP2MAN AND GZIP)
if(ENABLE_TOOLS AND GZIP)
	set(TOOLS 
		"ss_pak_extract" "ss_extract" "ss_cover_conv"
		"ss_adpcm_decode" "ss_ipu_conv" "ss_chc_decode"
		"gh_fsb_decrypt" "gh_xen_decrypt")
	set(MAN_SECTION "1")
	foreach(TOOL ${TOOLS})
		set(TOOL_MANFILE ${CMAKE_CURRENT_SOURCE_DIR}/${TOOL}.${MAN_SECTION})
		set(TOOL_MANFILE_GZ ${CMAKE_CURRENT_BINARY_DIR}/${TOOL}.${MAN_SECTION}.gz)
		add_custom_command(
			OUTPUT ${TOOL_MANFILE_GZ}
			COMMAND ${GZIP} -c ${TOOL_MANFILE} > ${TOOL_MANFILE_GZ}
			MAIN_DEPENDENCY ${TOOL_MANFILE}
			COMMENT "Building ${TOOL} man page"
			VERBATIM
		)
		add_custom_target(${TOOL}.${MAN_SECTION} ALL DEPENDS ${TOOL_MANFILE_GZ})
		install(FILES ${TOOL_MANFILE_GZ} DESTINATION share/man/man${MAN_SECTION})
	endforeach(TOOL)
endif(ENABLE_TOOLS AND GZIP)
