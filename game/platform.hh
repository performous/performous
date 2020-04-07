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
enum platforms { windows, linux, macos, bsd, solaris, unix };
Platform();
static platforms currentOS();
static uint16_t shortcutModifier(bool eitherSide = true);
static int defaultBackEnd();
};