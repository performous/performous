bin_PROGRAMS = ultrastarng

ultrastarng_SOURCES = main.cpp \
	audio.cpp \
	video.cpp \
	record.cpp \
	songs.cpp \
	lyrics.cpp \
	configuration.cpp \
	screenmanager.cpp \
	screen_intro.cpp \
	screen_songs.cpp \
	screen_sing.cpp \
	screen_practice.cpp \
	screen_score.cpp \
	screen_configuration.cpp \
	cairotosdl.cpp \
	pitch_graph.cpp \
	cairosvg.cpp \
	theme.cpp \
	sdl_gl.cpp \
	video_driver.cpp

INCLUDES = -I../include -I../audio\
	-O2 -Wall -Wextra -std=c++98 -pedantic -ansi \
	@CAIRO_FLAGS@ @XML_FLAGS@ @SDL_FLAGS@ \
	@VIDEO_FLAGS@ @SVG_FLAGS@ @AUDIO_FLAGS@ @PANGOCAIRO_FLAGS@\
	-DTHEMES_DIR="\"@THEMES_DIR@\"" -DDATA_DIR="\"@DATA_DIR@\""
LDADD   = @CAIRO_LIBS@ @XML_LIBS@ @SDL_LIBS@ \
	@VIDEO_LIBS@ @SVG_LIBS@ @AUDIO_LIBS@ @GL_LIBS@ @PANGOCAIRO_LIBS@ \
	-L../audio/.libs/ -lultrastarngrecord
