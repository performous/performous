#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>

#include <boost/predef/os.h>
#include <SDL_events.h>

#if defined(_MSC_VER)
#define NOMINMAX
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define _AMD64_
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(__i386__) || defined(_M_IX86)
#define _X86_
#elif defined(__arm__) || defined(_M_ARM) || defined(_M_ARMT)
#define _ARM_
#endif
#endif

#if (BOOST_OS_WINDOWS)
#include <handleapi.h>

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
	static std::unique_ptr<FILE, decltype(&fclose)> stdErrStream;
	static std::unique_ptr<HANDLE, decltype(&CloseHandle)> stdOutHandle;
#else
	static constexpr int stderr_fd = STDERR_FILENO;
#endif

  private:
#if (BOOST_OS_WINDOWS)
	void initWindowsConsole();
#endif
};
