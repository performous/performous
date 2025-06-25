cmake_minimum_required(VERSION 3.19)

set(PERFORMOUS_SEMVER "" CACHE STRING "Version in semver format")
set(PERFORMOUS_SHORT_VERSION "" CACHE STRING "Short version string")

set_target_properties(performous PROPERTIES
	OUTPUT_NAME "Performous"
	MACOSX_BUNDLE TRUE
	MACOSX_BUNDLE_BUNDLE_VERSION "${PERFORMOUS_SEMVER}"
	MACOSX_BUNDLE_SHORT_VERSION_STRING "${PERFORMOUS_SHORT_VERSION}"
	MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/resources/Info.plist.in"
	MACOSX_BUNDLE_COPYRIGHT "Performous ${PERFORMOUS_VERSION}, distributed by Performous team and contributors under the GPL 2.0")

set(PERFORMOUS_PATH "${CMAKE_PREFIX_PATH}")
list(GET
	PERFORMOUS_PATH
	0
	PERFORMOUS_SYSTEM_PREFIX
)

file(READ
	"${PERFORMOUS_SYSTEM_PREFIX}/etc/fonts/fonts.conf"
	PERFORMOUS_FONTS_CONF
)

string(REPLACE "<dir>${PERFORMOUS_SYSTEM_PREFIX}/share/fonts"
	"<dir prefix='relative'>../../../Resources/fonts" PERFORMOUS_FONTS_CONF
	"${PERFORMOUS_FONTS_CONF}")

string(REPLACE "<cachedir>${PERFORMOUS_SYSTEM_PREFIX}/var/cache"
	"<cachedir>~/Library/Caches/Performous" PERFORMOUS_FONTS_CONF
	"${PERFORMOUS_FONTS_CONF}")

string(REPLACE "<!-- Font directory list -->"
	[=[<!-- Font directory list -->
	<dir prefix='relative'>../../pixmaps</dir>]=] PERFORMOUS_FONTS_CONF
	"${PERFORMOUS_FONTS_CONF}")

string(REPLACE "${PERFORMOUS_SYSTEM_PREFIX}/share"
	"../../../Resources" PERFORMOUS_FONTS_CONF
	"${PERFORMOUS_FONTS_CONF}")

file(WRITE
	"${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_SYSCONFDIR}/fonts/fonts.conf"
	"${PERFORMOUS_FONTS_CONF}")

file(GLOB fontconfig_rules "${PERFORMOUS_SYSTEM_PREFIX}/etc/fonts/conf.d/*")
file(MAKE_DIRECTORY "${CMAKE_INSTALL_SYSCONFDIR}/fonts/conf.d")

# Iterate over the files and resolve symlinks
foreach(file_path ${fontconfig_rules})
	if(IS_SYMLINK "${file_path}")
		file(REAL_PATH "${file_path}" real_file_path)
		install(FILES "${real_file_path}" DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/fonts/conf.d/" )
	else()
		install(FILES "${file_path}" DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/fonts/conf.d/")
	endif()
endforeach()

install(TARGETS performous BUNDLE
	BUNDLE DESTINATION "${PERFORMOUS_BUNDLE_LOCATION}"
)

install(FILES "resources/performous.icns"
	TYPE DATA
)
