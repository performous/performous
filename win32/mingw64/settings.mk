# This variable controls the number of compilation processes
# within one package ("intra-package parallelism").
JOBS := 2
# This variable controls the targets that will build.
MXE_TARGETS := i686-w64-mingw32.shared
# This variable controls the download mirror for SourceForge,
# when it is used. Enabling the value below means auto.
#SOURCEFORGE_MIRROR := downloads.sourceforge.net
# The three lines below makes `make` build these "local
# packages" instead of all packages.
LOCAL_PKG_LIST := sdl2 boost portaudio ffmpeg portmidi pango gdk-pixbuf librsvg libsigc++ libxml++ opencv libepoxy jsoncpp cpp-netlib
.DEFAULT local-pkg-list:
local-pkg-list: $(LOCAL_PKG_LIST)