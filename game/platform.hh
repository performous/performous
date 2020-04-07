#pragma once

#include "fs.hh"
#include "log.hh"

#include <array>
#include <iostream>

#include <boost/predef/os.h>

#include <boost/filesystem.hpp>
#include <SDL2/SDL_events.h>

#if (BOOST_OS_WINDOWS) 
#include <sys/types.h>
#include <sys/stat.h>
#define _STAT(x, y) _stat(x, y)
#else
#include <sys/stat.h>
#define _STAT(x, y) stat(x, y)
#endif

struct Platform {
enum class Host_OS { Performous_OS_Win32, Performous_OS_Linux, Performous_OS_macOS, Performous_OS_BSD, Performous_OS_Solaris, Performous_OS_Unix };
Platform();
static Host_OS currentOS();
static uint16_t shortcutModifier(bool eitherSide = true);
static int defaultBackEnd();

};