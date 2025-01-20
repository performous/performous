#include "platform.hh"

#include "fs.hh"
#include "log.hh"

#include <fmt/format.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>

#if (BOOST_OS_WINDOWS)
#include <errhandlingapi.h>
#include <fcntl.h>
#include <IntSafe.h>
#include <ProcessEnv.h>
#include <wincon.h>

#elif (BOOST_OS_MACOS)
#include <CoreFoundation/CoreFoundation.h>

char * CFStringCopyUTF8String(CFStringRef aString) {
	if (aString == nullptr) {
		return nullptr;
	}

	CFIndex length = CFStringGetLength(aString);
	CFIndex maxSize =
	CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
	char *buffer = (char *)malloc((size_t)maxSize);
	if (buffer != nullptr) {
		if (CFStringGetCString(aString, buffer, maxSize, kCFStringEncodingUTF8)) {
			return buffer;
		}
		free(buffer); // If we failed
	}
	return nullptr;
}
#endif

Platform::HostOS Platform::currentOS() {
if constexpr(BOOST_OS_WINDOWS != 0) { return HostOS::OS_WIN; }
else if constexpr(BOOST_OS_LINUX != 0) { return HostOS::OS_LINUX; }
else if constexpr(BOOST_OS_MACOS != 0) { return HostOS::OS_MAC; }
else if constexpr(BOOST_OS_BSD != 0) { return HostOS::OS_BSD; }
else if constexpr(BOOST_OS_SOLARIS != 0) { return HostOS::OS_SOLARIS; }
else if constexpr(BOOST_OS_UNIX != 0) { return HostOS::OS_UNIX; }
}

std::uint16_t Platform::shortcutModifier(bool eitherSide) {
	if (currentOS() == HostOS::OS_MAC) { return eitherSide ? KMOD_GUI : KMOD_LGUI; }
	else { return eitherSide ? KMOD_CTRL : KMOD_LCTRL; }
}

Platform::Platform() {
#if (BOOST_OS_WINDOWS) 
	// set the locale to UTF-8 on windows
	_putenv_s("FONTCONFIG_PATH",".\\");
	setlocale(LC_ALL, ".UTF8");
	initWindowsConsole();
#elif (BOOST_OS_MACOS)
	CFURLRef resDirURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
	CFStringRef resDir = CFURLCopyPath(CFURLCopyAbsoluteURL(resDirURL));
	const char* pathStrBuffer = CFStringCopyUTF8String(resDir);
	if (pathStrBuffer == nullptr) throw std::logic_error("Failed to get a path for the app bundle.");
	fs::path resDirPath(pathStrBuffer);
	std::string defaultLang("en_US.UTF-8");
	CFPropertyListRef plist = CFPreferencesCopyAppValue(CFSTR("AppleLanguages"), kCFPreferencesCurrentApplication);
	try {
		CFArrayRef langs = (CFArrayRef)plist;
		if (CFArrayGetCount(langs) >= 1) {
			CFStringRef lang = (CFStringRef)CFArrayGetValueAtIndex(langs, 0);
			const char* langStrBuffer = CFStringCopyUTF8String(lang);
			if (langStrBuffer != nullptr) {
				defaultLang = langStrBuffer;
				defaultLang.replace(defaultLang.find("-"), 1, "_");
				defaultLang += ".UTF-8";
			}
		}
	}
	catch (std::exception const& e) {
		SpdLogger::error(LogSystem::LOGGER, "Language will default to en_US.UTF-8, due to error getting OS language from AppleDefaults. Exception={}.", e.what());
	}
	fs::path fcPath(resDirPath / "etc" / "fonts");
	fs::path pangoLibDir(resDirPath / "lib");
	fs::path pangoSysConfDir(resDirPath / "etc");
	setenv("LANG", defaultLang.c_str(), 1);
	setenv("FONTCONFIG_PATH", fcPath.u8string().c_str(), 1);
	setenv("PANGO_LIBDIR", pangoLibDir.u8string().c_str(), 1);
	setenv("PANGO_SYSCONFDIR", pangoSysConfDir.u8string().c_str(), 1);
	setenv("GDK_PIXBUF_MODULE_FILE", "", 1);
#endif
}

Platform::~Platform() {
#if (BOOST_OS_WINDOWS)
	FreeConsole();
#endif
}

int Platform::defaultBackEnd() {
		switch (Platform::currentOS()) {
			case HostOS::OS_WIN: return 13; // WASAPI
			case HostOS::OS_MAC: return 5; // CoreAudio
			case HostOS::OS_SOLARIS: return 7; // OSS
			case HostOS::OS_BSD: return 7; // OSS
			case HostOS::OS_LINUX: return 8; // ALSA
			case HostOS::OS_UNIX: return 8; // ALSA
			default: break;
		}
	throw std::runtime_error("Unable to determine a default Audio backend.");
}

#if (BOOST_OS_WINDOWS)
std::unique_ptr<FILE, decltype(&fclose)> Platform::stdErrStream{nullptr, fclose};
std::unique_ptr<HANDLE, decltype(&CloseHandle)> Platform::stdOutHandle{nullptr, CloseHandle};
int Platform::stderr_fd;

void Platform::initWindowsConsole() {
	if (AttachConsole(ATTACH_PARENT_PROCESS) == 0 && (fileno(stdout) == -2 || fileno(stderr) == -2)) {
		auto ptr = stdErrStream.get();
		freopen_s(&ptr, "NUL", "w", stderr);
	}
	else {
		freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
		freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
		HANDLE hStdout = CreateFile(
			"CONOUT$", GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
		);
		if (hStdout == INVALID_HANDLE_VALUE) {
			fmt::print(stderr, "Unable to get a handle to the stdout console. Error={}", GetLastError());
		}
		else {
			stdOutHandle.reset(&hStdout);
			SetStdHandle(STD_OUTPUT_HANDLE, *stdOutHandle);
			std::setvbuf(stdout, nullptr, _IONBF, 0);
			std::cout.clear();
			std::wcout.clear();
		}
	}
	stderr_fd = fileno(stderr);
}

extern "C" {
// For DWORD (see end of file)
// Force high-performance graphics on dual-GPU systems
	// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	// https://community.amd.com/thread/169965
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif