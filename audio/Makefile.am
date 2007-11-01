EXTRA_DIST = alsa/alsa.hpp audio.hpp audio_dev.hpp fft.hpp sample.hpp plugin.hpp

lib_LTLIBRARIES = libdausng.la

flags = -I$(top_srcdir)/
lib = -lboost_thread
sources = audio.cpp audio_dev_tone.cpp

if RECORD_ALSA
sources += audio_dev_alsa.cpp
flags +=  @RECORD_ALSA_FLAGS@
lib += @RECORD_ALSA_LIBS@
endif

if RECORD_PA18
sources += audio_dev_pa18.cpp
flags +=  @RECORD_PA18_FLAGS@
lib += @RECORD_PA18_LIBS@
endif

if RECORD_PA19
sources += audio_dev_pa.cpp
flags +=  @RECORD_PA19_FLAGS@
lib += @RECORD_PA19_LIBS@
endif

if RECORD_GST
sources += audio_dev_gst.cpp
flags +=  @RECORD_GST_FLAGS@
lib += @RECORD_GST_LIBS@
endif

libdausng_la_SOURCES = $(sources)
INCLUDES = $(flags)
libdausng_la_LIBADD  = $(lib)

