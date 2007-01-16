bin_PROGRAMS = ultrastarng

ultrastarng_SOURCES = main.cpp audio.cpp record.cpp songs.cpp screenmanager.cpp screen_intro.cpp \
	screen_songs.cpp screen_sing.cpp cairotosdl.cpp pitch_graph.cpp cairosvg.cpp theme.cpp

INCLUDES = -I../include @CAIRO_FLAGS@ @XML_FLAGS@ @SDL_FLAGS@ @FFT_FLAGS@ @ALSA_FLAGS@
LDADD = @CAIRO_LIBS@ @XML_LIBS@ @SDL_LIBS@ @FFT_LIBS@ @ALSA_LIBS@
#ultarstar_CPPFLAGS = -I../include
#ultarstar_CXXFLAGS = -I../include
#ultarstar_LDADD = -DTA_WATCHDOG @cc_debug_option@
#ultrastar_LIBADD  = 
