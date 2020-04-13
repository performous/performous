#include "platform.hh"
#include "fs.hh"

Platform::platforms Platform::currentOS() {
if (BOOST_OS_WINDOWS != 0) { return platforms::windows; }
else if (BOOST_OS_LINUX != 0) { return platforms::linux; }
else if (BOOST_OS_MACOS != 0) { return platforms::macos; }
else if (BOOST_OS_BSD != 0) { return platforms::bsd; }
else if (BOOST_OS_SOLARIS != 0) { return platforms::solaris; }
else if (BOOST_OS_UNIX != 0) { return platforms::unix; }
}

uint16_t Platform::shortcutModifier(bool eitherSide) {
	if (currentOS() == platforms::macos) { return eitherSide ? KMOD_GUI : KMOD_LGUI; }
	else { return eitherSide ? KMOD_CTRL : KMOD_LCTRL; }
}

Platform::Platform() {
	#if (BOOST_OS_WINDOWS)
	_putenv_s("FONTCONFIG_PATH",".\\etc\\");
	#endif
}

const std::array<const char*,6> Platform::platformNames = {{ "Windows", "Linux", "MacOS", "BSD", "Solaris", "Unix" }}; // Relevant for debug only.

int Platform::defaultBackEnd() {
		switch (Platform::currentOS()) {
			case platforms::windows: return 13; // WASAPI
			case platforms::macos: return 5; // CoreAudio
			case platforms::solaris: return 7; // OSS
			case platforms::bsd: return 7; // OSS
			case platforms::linux: return 8; // ALSA
			case platforms::unix: return 8; // ALSA
			default: break;
		}
	throw std::runtime_error("Unable to determine a default Audio backend.");
}

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