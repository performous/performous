cmake_minimum_required(VERSION 3.15)
project(Performous CXX C)

set(CMAKE_VERBOSE_MAKEFILE TRUE CACHE BOOL "Produce verbose makefile" FORCE)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

if(NOT PERFORMOUS_VERSION)
    message("No project version specified. Determining latest tag..")
    include(GetGitRevisionDescription)
    git_describe(PROJECT_VERSION --tags)
    if(NOT PROJECT_VERSION)
        message("No project version found. Falling back onto default versioning strategy.")
        set(PROJECT_VERSION "1.3.1")
    endif()
else()
    set(PROJECT_VERSION "${PERFORMOUS_VERSION}")
endif()
string(TIMESTAMP YEAR "%Y")

message("Building ${PROJECT_NAME} ${PROJECT_VERSION}")

# Avoid source tree pollution
get_filename_component(REAL_SOURCE_DIR "${CMAKE_SOURCE_DIR}" REALPATH)
get_filename_component(REAL_BUILD_DIR "${CMAKE_BINARY_DIR}" REALPATH)
if(REAL_SOURCE_DIR STREQUAL REAL_BUILD_DIR)
	message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files already created:\nrm -rf CMakeCache.txt CMakeFiles")
endif(REAL_SOURCE_DIR STREQUAL REAL_BUILD_DIR)

# Add a sensible build type default and warning because empty means no optimization and no debug info.
if(NOT DEFINED CMAKE_BUILD_TYPE)
	message("WARNING: CMAKE_BUILD_TYPE is not defined!\n         Defaulting to CMAKE_BUILD_TYPE=RelWithDebInfo. Use ccmake or the command-line to set a proper value.")
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT DEFINED CMAKE_BUILD_TYPE)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_definitions("-DNDEBUG")
endif()

if(WIN32)
	if (MINGW)
		set(CPACK_MINGW_SYSTEM_PREFIX "C:/msys64/mingw64" CACHE PATH "Path to the mingw-w64 system root.")
	endif()
	set(CMAKE_CXX_FLAGS "-DWIN32_LEAN_AND_MEAN ${CMAKE_CXX_FLAGS}")
	set(CMAKE_INSTALL_DATAROOTDIR ".")
	set(CMAKE_INSTALL_LOCALEDIR "locale")
elseif(NOT APPLE)
	set(CMAKE_INSTALL_DATAROOTDIR "share/games/performous")
	set(CMAKE_INSTALL_LOCALEDIR "share/locale")
elseif(APPLE)
	set(PERFORMOUS_BUNDLE_LOCATION "${CMAKE_INSTALL_PREFIX}" CACHE PATH "Location to put the Performous app bundle in." FORCE)
	mark_as_advanced(PERFORMOUS_BUNDLE_LOCATION)
	# make sure CMAKE_INSTALL_PREFIX ends in /
	if(NOT CMAKE_INSTALL_PREFIX MATCHES "/$")
		string(APPEND CMAKE_INSTALL_PREFIX "/")
	set (CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE PATH "Location to put the Performous app bundle in.")
	endif()
	string(APPEND CMAKE_INSTALL_PREFIX "Performous.app")
	set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
	set(CMAKE_INSTALL_DATAROOTDIR "Contents/Resources")
	set(CMAKE_INSTALL_DATADIR "${CMAKE_INSTALL_DATAROOTDIR}")
	set(CMAKE_INSTALL_LOCALEDIR "${CMAKE_INSTALL_DATADIR}/Locale")
	set(CMAKE_INSTALL_SYSCONFDIR "${CMAKE_INSTALL_DATADIR}/etc")
	set(CMAKE_INSTALL_BINDIR "Contents/MacOS")
	set(CMAKE_INSTALL_MANDIR "${CMAKE_INSTALL_DATADIR}/man/man6")
endif()

set(CMAKE_CXX_FLAGS_RelWithDebInfo "${CMAKE_CXX_FLAGS_RelWithDebInfo} -O3")

set(SHARE_INSTALL "${CMAKE_INSTALL_DATAROOTDIR}" CACHE INTERNAL "Data file install path. Must be a relative path (from CMAKE_INSTALL_PREFIX), with no trailing slash.")
mark_as_advanced(SHARE_INSTALL)

set(LOCALE_DIR "${CMAKE_INSTALL_LOCALEDIR}" CACHE INTERNAL "Locale file install path. Must be a relative path (from CMAKE_INSTALL_PREFIX), with no trailing slash.")
mark_as_advanced(LOCALE_DIR)

find_package(Gettext)
if(Gettext_FOUND)
	message(STATUS "Localization enabled: Building and installing .mo files")
	add_subdirectory(lang)
else()
	message(STATUS "Localization disabled: Gettext tools (msgfmt) missing")
endif()

add_subdirectory(data)

add_subdirectory(game)

target_compile_options(performous PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wextra> $<$<CXX_COMPILER_ID:gcc>:-fcx-limited-range> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wconversion>)


add_subdirectory(docs)

if(APPLE)
	add_subdirectory(osx-utils)
endif()

if(NOT APPLE)
  # uninstall target
  if(NOT TARGET uninstall)
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake"
      IMMEDIATE @ONLY)

    add_custom_target(uninstall
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake/uninstall.cmake)
  endif()
endif()

if(WIN32)
	set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
endif()

#CPACK variables
include(InstallRequiredSystemLibraries)

# Project definition
set(CPACK_PACKAGE_VENDOR "Performous")
set(CPACK_PACKAGE_DESCRIPTION "Karaoke, band games and dancing in the same package. Performous lets you perform great songs, improve your musical skills and simply have fun! No special hardware required, you may play guitar on your PC keyboard, sing on your laptop microphone and so on. If you do have SingStar microphones, Guitar Hero or Rock Band instruments, or dance pads, just plug them into your USB ports and start rocking!")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An open-source music and rhythm game.")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
if(WIN32)
	set(CPACK_PACKAGE_EXECUTABLES "Performous" "${CMAKE_CURRENT_SOURCE_DIR}/win32\\\\performous.ico")
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "Performous\\\\Performous")
else()
	set(CPACK_PACKAGE_EXECUTABLES "performous")
endif()
set(CPACK_PACKAGE_CONTACT "Arjan Speiard | Asgard Sings! <info@asgardsings.nl>")

if(NOT CPACK_PACKAGE_VERSION)
	set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
endif()
set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${CMAKE_BINARY_DIR};${PROJECT_NAME};ALL;/")

set(BUILD_TESTS AUTO CACHE STRING "Build tests [OFF|AUTO*|ON]")
set_property(CACHE BUILD_TESTS PROPERTY STRINGS AUTO ON OFF)


if(BUILD_TESTS STREQUAL "AUTO" OR BUILD_TESTS STREQUAL "ON")
	enable_testing()
	add_subdirectory(testing)
endif()

# Debian specific settings
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SECTION "Games")

# RPM specific settings
set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")

set(CPACK_RPM_PACKAGE_REQUIRES "cairo-devel, librsvg2-devel, glibmm24-devel, libxml++-devel, boost-devel, SDL2-devel, libepoxy-devel, ffmpeg-devel, portaudio-devel, opencv-devel, portmidi-devel, libjpeg-turbo-devel, pango-devel, libpng-devel, glm-devel, openblas-devel, fftw-devel, cpprest-devel")
set(CPACK_RPM_PACKAGE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")

set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "${LC_MESSAGES_DIRS}")
set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION}")
set(CPACK_RPM_PACKAGE_URL "https://github.com/performous/performous")

# NSIS specific settings
set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/win32\\\\performous.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/win32\\\\performous.ico")
set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CMAKE_CURRENT_SOURCE_DIR}/win32/nsis\\\\welcomefinishpage.bmp")
set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MODIFY_PATH OFF)
set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
set(CPACK_NSIS_PACKAGE_NAME "${CPACK_NSIS_DISPLAY_NAME}")
set(CPACK_NSIS_INSTALLED_ICON_NAME "${PROJECT_NAME}.exe")
set(CPACK_NSIS_HELP_LINK "https://github.com/performous/performous")
set(CPACK_NSIS_URL_INFO_ABOUT "https://performous.org")
set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${PROJECT_NAME}.lnk' '$INSTDIR\\\\${PROJECT_NAME}.exe'"
								  "CreateShortcut '$SMPROGRAMS\\\\Performous\\\\Uninstall.lnk' '$INSTDIR\\\\Uninstall.exe''")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMPROGRAMS\\\\$START_MENU\\\\${PROJECT_NAME}.lnk'"
								  "Delete '$SMPROGRAMS\\\\$START_MENU\\\\Uninstall.lnk'")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${PROJECT_NAME}.exe")
set(CPACK_NSIS_MENU_LINKS "https://discord.gg/NS3m3ad" "Performous Discord")
set(CPACK_NSIS_WELCOME_TITLE "Welcome to the Performous installer.")
set(CPACK_NSIS_FINISH_TITLE "Congratulations Performous is now installed.")
set(CPACK_NSIS_MUI_HEADERIMAGE "${CMAKE_CURRENT_SOURCE_DIR}/win32/nsis\\\\headerimage.bmp")
set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)
set(CPACK_NSIS_BRANDING_TEXT "Performous installer ${PROJECT_VERSION} 2009-${YEAR}")

# macOS specific settings
set(CPACK_BUNDLE_NAME "Performous")
set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/osx-utils/resources/Info.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/osx-utils/resources/Performous.icns")

if (APPLE)
	set(CPACK_SOURCE_GENERATOR "")
	set(CPACK_GENERATOR "Bundle")
	set(CMAKE_XCODE_ATTRIBUTE_MARKETING_VERSION "${PROJECT_VERSION}")
	set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.performous.Performous")
	set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
	set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
elseif(WIN32)
	set(CPACK_GENERATOR "NSIS64;TGZ;ZIP")
else()
	set(CPACK_GENERATOR "TGZ;ZIP;DEB;RPM;NSIS64")
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	set (CPACK_STRIP_FILES TRUE)
endif()

include(CPack)
