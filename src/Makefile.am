bin_PROGRAMS = ultrastarng

ultrastarng_SOURCES = main.cpp \
	audio.cpp \
	video.cpp \
	record.cpp \
	songs.cpp \
	lyrics.cpp \
	screenmanager.cpp \
	screen_intro.cpp \
	screen_songs.cpp \
	screen_sing.cpp \
	cairotosdl.cpp \
	pitch_graph.cpp \
	cairosvg.cpp \
	theme.cpp \
	sdl_gl.cpp \
	video_driver.cpp

INCLUDES = -I../include \
	@CAIRO_FLAGS@ @XML_FLAGS@ @SDL_FLAGS@ @FFT_FLAGS@ @ALSA_FLAGS@ \
	@VIDEO_FLAGS@ @SVG_FLAGS@ @AUDIO_FLAGS@ \
	-DTHEMES_DIR="\"@THEMES_DIR@\"" -DDATA_DIR="\"@DATA_DIR@\""
LDADD   = @CAIRO_LIBS@ @XML_LIBS@ @SDL_LIBS@ @FFT_LIBS@ @ALSA_LIBS@ \
	@VIDEO_LIBS@ @SVG_LIBS@ @AUDIO_LIBS@ @GL_LIBS@
