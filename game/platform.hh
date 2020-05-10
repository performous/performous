#pragma once

#include "fs.hh"
#include "log.hh"

#include <array>
#include <iostream>

#if ((BOOST_VERSION / 100 % 1000) >= 55)
#include <boost/predef/os.h>
#else
#include "../boost_predef/os.h"
#endif

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

private:
static const std::array<const char*,6> platformNames;
};

#if (BOOST_OS_WINDOWS)
extern "C" {
// For DWORD (see end of file)
#include "windef.h"
// Force high-performance graphics on dual-GPU systems
	// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	// https://community.amd.com/thread/169965
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif