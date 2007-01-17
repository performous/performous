bin_PROGRAMS = ultrastarng

ultrastarng_SOURCES = main.cpp \
	audio.cpp \
	record.cpp \
	songs.cpp \
	screenmanager.cpp \
	screen_intro.cpp \
	screen_songs.cpp \
	screen_sing.cpp \
	cairotosdl.cpp \
	pitch_graph.cpp \
	cairosvg.cpp \
	theme.cpp \
	../include/audio.h \
	../include/cairotosdl.h \
	../include/pitch_graph.h \
	../include/screen.h \
	../include/screen_sing.h \
	../include/singleton.h \
	../include/theme.h \
	../include/cairosvg.h \
	../include/record.h \
	../include/screen_intro.h \
	../include/screen_songs.h \
	../include/songs.h

INCLUDES = -I../include @CAIRO_FLAGS@ @XML_FLAGS@ @SDL_FLAGS@ @FFT_FLAGS@ @ALSA_FLAGS@ @VIDEO_FLAGS@ @SVG_FLAGS@ @AUDIO_FLAGS@
LDADD = @CAIRO_LIBS@ @XML_LIBS@ @SDL_LIBS@ @FFT_LIBS@ @ALSA_LIBS@ @VIDEO_LIBS@ @SVG_LIBS@ @AUDIO_LIBS@
#ultarstar_CPPFLAGS = -I../include
#ultarstar_CXXFLAGS = -I../include
#ultarstar_LDADD = -DTA_WATCHDOG @cc_debug_option@
#ultrastar_LIBADD  = 
