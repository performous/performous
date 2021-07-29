#pragma once

#include "log.hh"

#include <array>
#include <iostream>

#include <boost/predef/os.h>

#include <SDL2/SDL_events.h>

#if (BOOST_OS_WINDOWS) 
#include <sys/types.h>
#include <sys/stat.h>
#define _STAT(x, y) _stat(x, y)
using STAT = struct _stat;
#else
#include <sys/stat.h>
#define _STAT(x, y) stat(x, y)
using STAT = struct stat;
#endif

struct Platform {
enum platforms { windows, linux, macos, bsd, solaris, unix };
Platform();
static platforms currentOS();
static uint16_t shortcutModifier(bool eitherSide = true);
static int defaultBackEnd();

private:
static const std::array<const char*,6> platformNames;
};
