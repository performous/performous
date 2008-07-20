EXTRA_DIST = alsa/alsa.hpp audio.hpp audio_dev.hpp fft.hpp sample.hpp plugin.hpp

lib_LTLIBRARIES = libdausng.la

flags = -Wshadow -Wextra -Wall -O2

lib = -lboost_thread
sources = audio.cpp audio_dev_tone.cpp

if LIBDA_ALSA
sources += audio_dev_alsa.cpp
flags +=  @LIBDA_ALSA_FLAGS@
lib += @LIBDA_ALSA_LIBS@
endif

if LIBDA_PA18
sources += audio_dev_pa18.cpp
flags +=  @LIBDA_PA18_FLAGS@
lib += @LIBDA_PA18_LIBS@
endif

if LIBDA_PA19
sources += audio_dev_pa19.cpp
flags +=  @LIBDA_PA19_FLAGS@
lib += @LIBDA_PA19_LIBS@
endif

if LIBDA_GST
sources += audio_dev_gst.cpp
flags +=  @LIBDA_GST_FLAGS@
lib += @LIBDA_GST_LIBS@
endif

if LIBDA_JACK
sources += audio_dev_jack.cpp
flags +=  @LIBDA_JACK_FLAGS@
lib += @LIBDA_JACK_LIBS@
endif

libdausng_la_SOURCES = $(sources)
INCLUDES = $(flags)
libdausng_la_LIBADD  = $(lib)

