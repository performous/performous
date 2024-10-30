#pragma once

#include <array>
#include <cstdint>
#include <iostream>

#include <boost/predef/os.h>

#include <SDL_events.h>

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
enum class HostOS { OS_WIN, OS_LINUX, OS_MAC, OS_BSD, OS_SOLARIS, OS_UNIX };
Platform();
static HostOS currentOS();
static std::uint16_t shortcutModifier(bool eitherSide = true);
static int defaultBackEnd();
static void setupPlatform();

private:
static const std::array<const char*,6> platformNames;
};
