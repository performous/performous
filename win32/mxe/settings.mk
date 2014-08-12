JOBS := 1
MXE_TARGETS :=  i686-w64-mingw32.shared
LOCAL_PKG_LIST := gettext sdl sdl2 boost portaudio ffmpeg portmidi pango gdk-pixbuf librsvg libsigc++ libxml++ glew libcdio opencv
.DEFAULT local-pkg-list:
local-pkg-list: $(LOCAL_PKG_LIST)
