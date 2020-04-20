# - Try to find PortMidi
# Once done, this will define
#
#  PortMidi_FOUND - system has PortMidi
#  PortMidi_INCLUDE_DIRS - the PortMidi include directories
#  PortMidi_LIBRARIES - link these to use PortMidi
#  PortMidi_VERSION - detected version of PortMidi
#
# See documentation on how to write CMake scripts at
# http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries

include(LibFindMacros)

find_path(PortMidi_INCLUDE_DIR NAMES portmidi.h)
find_library(PortMidi_LIBRARY NAMES portmidi)
find_library(PortTime_LIBRARY NAMES porttime)

set(PortMidi_PROCESS_INCLUDES PortMidi_INCLUDE_DIR)
set(PortMidi_PROCESS_LIBS PortMidi_LIBRARY)
# Porttime library is merged to Portmidi in new versions, so
# we work around problems by adding it only if it's present
if (${PortTime_LIBRARY})
	set(PortMidi_PROCESS_LIBS PortMidi_PROCESS_LIBS PortTime_LIBRARY)
endif (${PortTime_LIBRARY})

libfind_process(PortMidi)
mark_as_advanced(PortTime_LIBRARY)
