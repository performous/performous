cmake_minimum_required(VERSION 2.8)

# It should be possible to switch to CMake's GETTEXT_CREATE_TRANSLATIONS
# if we either drop the LOCALE_DIR or that function gets a customised
# INSTALL_DESTINATION.
# FIXME: Always execute msgmerge, like in CMake's GETTEXT_CREATE_TRANSLATIONS?

file(GLOB _languages *.po)
set(_mofiles)
foreach(_pofile ${_languages})
	get_filename_component(_language ${_pofile} NAME_WE)
	set(_mofile ${CMAKE_CURRENT_BINARY_DIR}/${_language}.mo)
	add_custom_command(
		OUTPUT ${_mofile}
		COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -v ${_pofile} -o ${_mofile}
		DEPENDS ${_pofile}
		COMMENT "Building ${_language} locale"
		VERBATIM)
	install(FILES ${_mofile} DESTINATION ${LOCALE_DIR}/${_language}/LC_MESSAGES RENAME ${CMAKE_PROJECT_NAME}.mo)
	set(_mofiles ${_mofiles} ${_mofile})
endforeach()

add_custom_target(translations ALL DEPENDS ${_mofiles})

