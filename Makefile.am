SUBDIRS = src

themesdir   = @THEMES_DIR@/default
themes_DATA = themes/default/intro.svg \
	themes/default/songs.svg \
	themes/default/sing_bg.svg \
	themes/default/sing_lyricscurrent.svg \
	themes/default/sing_lyricshighlight.svg \
	themes/default/sing_lyricsnext.svg \
	themes/default/sing_p1box.svg \
	themes/default/sing_p1score.svg \
	themes/default/sing_p2box.svg \
	themes/default/sing_p2score.svg \
	themes/default/sing_progressfg.svg \
	themes/default/sing_timetxt.svg

imagesdir   = @IMAGES_DIR@
images_DATA = images/no_cover.png

fontsdir    = @FONTS_DIR@
fonts_DATA  = fonts/DejaVuSansCondensed.ttf
	
includes = include/audio.h \
	include/cairotosdl.h \
	include/pitch_graph.h \
	include/screen.h \
	include/screen_sing.h \
	include/singleton.h \
	include/theme.h \
	include/cairosvg.h \
	include/record.h \
	include/screen_intro.h \
	include/screen_songs.h \
	include/songs.h

EXTRA_DIST = $(themes_DATA) $(images_DATA) $(fonts_DATA) $(includes)
