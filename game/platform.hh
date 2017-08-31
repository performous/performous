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

struct Platform {
enum platforms { windows, linux, macos, bsd, solaris, unix };
Platform();
static platforms currentOS();

private:
static const std::array<const char*,6> platformNames;
};