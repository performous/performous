set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${Plugin++_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY " Plugin++ is a cross platform plug-in framework for C++ that uses XML to marshal data across the application and the shared library space.")
set(CPACK_PACKAGE_CONTACT "http://sourceforge.net/projects/pluginpp/")
set(CPACK_SOURCE_IGNORE_FILES
   "/.cvsignore"
   "/.gitignore"
   "/songs/"
   "/build/"
   "/.svn/"
   "/.git/"
   "/osx-utils/"
   "/portage-overlay/"
)


if("${CMAKE_BUILD_TYPE}" MATCHES "Release")
	set(CPACK_STRIP_FILES TRUE)
endif("${CMAKE_BUILD_TYPE}" MATCHES "Release")


if(UNIX)
	# Try to find architecture
	execute_process(COMMAND uname -m OUTPUT_VARIABLE CPACK_PACKAGE_ARCHITECTURE)
	string(STRIP "${CPACK_PACKAGE_ARCHITECTURE}" CPACK_PACKAGE_ARCHITECTURE)
	# Try to find distro name and distro-specific arch
	execute_process(COMMAND lsb_release -is OUTPUT_VARIABLE LSB_ID)
	execute_process(COMMAND lsb_release -rs OUTPUT_VARIABLE LSB_RELEASE)
	string(STRIP "${LSB_ID}" LSB_ID)
	string(STRIP "${LSB_RELEASE}" LSB_RELEASE)
	set(LSB_DISTRIB "${LSB_ID}${LSB_RELEASE}")
	if(NOT LSB_DISTRIB)
		set(LSB_DISTRIB "unix")
	endif(NOT LSB_DISTRIB)
	# For Debian-based distros we want to create DEB packages.
	if("${LSB_DISTRIB}" MATCHES "Ubuntu|Debian")
		set(CPACK_GENERATOR "DEB")
		set(CPACK_DEBIAN_PACKAGE_PRIORITY "extra")
		set(CPACK_DEBIAN_PACKAGE_SECTION "universe/games")
		set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "ultrastar-songs, ultrastar-songs-restricted, msttcorefonts")
		# We need to alter the architecture names as per distro rules
		if("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "i[3-6]86")
			set(CPACK_PACKAGE_ARCHITECTURE i386)
		endif("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "i[3-6]86")
		if("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "x86_64")
			set(CPACK_PACKAGE_ARCHITECTURE amd64)
		endif("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "x86_64")
		# Set the dependencies based on the distro version
		if("${LSB_DISTRIB}" MATCHES "Debian5.*")
			set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-dev, libc6-dev")
		endif("${LSB_DISTRIB}" MATCHES "Debian5.*")
		if(NOT CPACK_DEBIAN_PACKAGE_DEPENDS)
			message("WARNING: ${LSB_DISTRIB} not supported yet.\nPlease set deps in cmake/performous-packaging.cmake before packaging.")
		endif(NOT CPACK_DEBIAN_PACKAGE_DEPENDS)
	endif("${LSB_DISTRIB}" MATCHES "Ubuntu|Debian")
	set(CPACK_SYSTEM_NAME "${LSB_DISTRIB}-${CPACK_PACKAGE_ARCHITECTURE}")
	message(STATUS "Detected ${CPACK_SYSTEM_NAME}. Use make package to build packages (${CPACK_GENERATOR}).")
endif(UNIX)

include(CPack)

