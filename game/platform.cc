#include "platform.hh"
#include "fs.hh"

Platform::platforms Platform::currentOS() {
if (BOOST_OS_WINDOWS != 0) { return windows; }
else if (BOOST_OS_LINUX != 0) { return linux; }
else if (BOOST_OS_MACOS != 0) { return macos; }
else if (BOOST_OS_BSD != 0) { return bsd; }
else if (BOOST_OS_SOLARIS != 0) { return solaris; }
else if (BOOST_OS_UNIX != 0) { return unix; }
}

uint16_t Platform::shortcutModifier(bool eitherSide) {
	if (currentOS() == macos) { return eitherSide ? KMOD_GUI : KMOD_LGUI; }
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
			case windows: return 13; // WASAPI
			case macos: return 5; // CoreAudio
			case solaris: return 7; // OSS
			case bsd: return 7; // OSS
			case linux: return 8; // ALSA
			case unix: return 8; // ALSA
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