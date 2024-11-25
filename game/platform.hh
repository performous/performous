#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>

#include <boost/predef/os.h>
#include <SDL_events.h>

#if (BOOST_OS_WINDOWS) 
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#define _STAT(x, y) _stat(x, y)
using STAT = struct _stat;
#else
#include <sys/stat.h>
#include <unistd.h>
#define _STAT(x, y) stat(x, y)
using STAT = struct stat;
#endif

struct Platform {
	Platform();
	~Platform();

	enum class HostOS { OS_WIN, OS_LINUX, OS_MAC, OS_BSD, OS_SOLARIS, OS_UNIX };

	static HostOS currentOS();
	static std::uint16_t shortcutModifier(bool eitherSide = true);
	static int defaultBackEnd();
	
#if (BOOST_OS_WINDOWS)
	static int stderr_fd;
  private:
	std::unique_ptr<FILE, decltype(&fclose)> stdErrStream{nullptr, fclose};
#else
	static constexpr int stderr_fd = STDERR_FILENO;
#endif

  private:
#if (BOOST_OS_WINDOWS)
	void initWindowsConsole();
#endif
};
