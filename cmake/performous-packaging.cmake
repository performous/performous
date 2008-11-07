set(CPACK_GENERATOR "TBZ2;DEB")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_PACKAGE_NAME "Performous")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A karaoke game with pitch detection and scoring, similar to Singstar games. Previously known as UltraStar-NG. Supports songs in Ultrastar format.")
set(CPACK_PACKAGE_CONTACT "http://performous.org/")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 3)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_SOURCE_IGNORE_FILES
   "/.cvsignore"
   "/songs/"
   "/build/"
   "/.svn/"
   "/osx-utils/"
   "/portage-overlay/"
)
# These deps are for Ubuntu 8.04
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl1.2debian, libcairo2, librsvg2-2, libboost-thread1.34.1, libboost-serialization1.34.1, libboost-program-options1.34.1, libboost-regex1.34.1, libboost-filesystem1.34.1, libavcodec1d, libavformat1d, libswscale1d, libmagick++10, libsamplerate0, libxml++2.6c2a")
# These deps are for Ubuntu 8.10
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl1.2debian, libcairo2, librsvg2-2, libboost-thread1.34.1, libboost-serialization1.34.1, libboost-program-options1.34.1, libboost-regex1.34.1, libboost-filesystem1.34.1, libavcodec51, libavformat52, libswscale0, libmagick++10, libsamplerate0, libxml++2.6-2")
set(CPACK_DEBIAN_PACKAGE_PRIORITY extra)
set(CPACK_DEBIAN_PACKAGE_SECTION universe/games)
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS ultrastar-songs)
include(CPack)

