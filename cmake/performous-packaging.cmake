set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A karaoke, band and dancing game where one or more players perform a song and the game scores their performances. Supports songs in UltraStar, Frets on Fire and StepMania formats. Microphones and instruments from SingStar, Guitar Hero and Rock Band as well as some dance pads are autodetected.")
set(CPACK_PACKAGE_CONTACT "http://performous.org/")
set(CPACK_SOURCE_IGNORE_FILES
   "/.cvsignore"
   "/.gitignore"
   "/songs/"
   "/build/"
   "/.svn/"
   "/.git/"
   "/osx-utils/"
   "/portage-overlay/"
   "/win32/"
)
set(CPACK_PACKAGE_EXECUTABLES performous)
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_GENERATOR "TBZ2")

if("${CMAKE_BUILD_TYPE}" MATCHES "Release")
	set(CPACK_STRIP_FILES TRUE)
endif("${CMAKE_BUILD_TYPE}" MATCHES "Release")

if(APPLE)
	set(CPACK_GENERATOR "PACKAGEMAKER;OSXX11")
endif(APPLE)
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
		if("${LSB_DISTRIB}" MATCHES "Ubuntu9.10")
			set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl1.2debian, libcairo2, librsvg2-2, libboost-thread1.38.0, libboost-program-options1.38.0, libboost-regex1.38.0, libboost-filesystem1.38.0, libboost-date-time1.38.0, libavcodec52, libavformat52, libswscale0, libmagick++2, libxml++2.6-2, libglew1.5, libpng12-0, libjpeg62")
		endif("${LSB_DISTRIB}" MATCHES "Ubuntu9.10")
		if("${LSB_DISTRIB}" MATCHES "Debian5.*")
			set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl1.2debian, libcairo2, librsvg2-2, libboost-dev, libavcodec51, libavformat52, libswscale0, libmagick++10, libxml++2.6-2, libglew1.5")
		endif("${LSB_DISTRIB}" MATCHES "Debian5.*")
		if(NOT CPACK_DEBIAN_PACKAGE_DEPENDS)
			message("WARNING: ${LSB_DISTRIB} not supported yet.\nPlease set deps in cmake/performous-packaging.cmake before packaging.")
		endif(NOT CPACK_DEBIAN_PACKAGE_DEPENDS)
	endif("${LSB_DISTRIB}" MATCHES "Ubuntu|Debian")
	# For Fedora-based distros we want to create RPM packages.
	if("${LSB_DISTRIB}" MATCHES "Fedora")
		set(CPACK_GENERATOR "RPM")
		# We need to alter the architecture names as per distro rules
		if("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "i[3-6]86")
			set(CPACK_PACKAGE_ARCHITECTURE i386)
		endif("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "i[3-6]86")
		if("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "x86_64")
			set(CPACK_PACKAGE_ARCHITECTURE amd64)
		endif("${CPACK_PACKAGE_ARCHITECTURE}" MATCHES "x86_64")
		# Set the dependencies based on the distro version
		if("${LSB_DISTRIB}" MATCHES "Fedora12")
			set(CPACK_RPM_PACKAGE_REQUIRES "gettext, gtk2, cairo, librsvg2, libsigc++20, glibmm24, libxml++, ImageMagick-c++, boost, SDL, glew, ffmpeg, pulseaudio-libs")
		endif("${LSB_DISTRIB}" MATCHES "Fedora12")
		if(NOT CPACK_RPM_PACKAGE_REQUIRES)
			message("WARNING: ${LSB_DISTRIB} not supported yet.\nPlease set deps in cmake/performous-packaging.cmake before packaging.")
		endif(NOT CPACK_RPM_PACKAGE_REQUIRES)
	endif("${LSB_DISTRIB}" MATCHES "Fedora")
	set(CPACK_SYSTEM_NAME "${LSB_DISTRIB}-${CPACK_PACKAGE_ARCHITECTURE}")
	message(STATUS "Detected ${CPACK_SYSTEM_NAME}. Use make package to build packages (${CPACK_GENERATOR}).")
endif(UNIX)

if(WIN32)
  set(CPACK_GENERATOR "NSIS")
  set(CPACK_PACKAGE_EXECUTABLES "performous" "Performous - game")#executable file - display name to menu programs
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/themes/default\\\\icon.bmp")
  SET(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/themes/default\\\\icon.ico")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\${CMAKE_PROJECT_NAME}.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} Performous!")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\performous.org")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\performous.org")
  SET(CPACK_NSIS_MODIFY_PATH OFF)

endif(WIN32)

include(CPack)

