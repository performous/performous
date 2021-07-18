#include "platform.hh"
#include "fs.hh"

Platform::HostOS Platform::currentOS() {
if (BOOST_OS_WINDOWS != 0) { return PERFORMOUS_WIN; }
else if (BOOST_OS_LINUX != 0) { return PERFORMOUS_LINUX; }
else if (BOOST_OS_MACOS != 0) { return PERFORMOUS_MAC; }
else if (BOOST_OS_BSD != 0) { return PERFORMOUS_BSD; }
else if (BOOST_OS_SOLARIS != 0) { return PERFORMOUS_SOLARIS; }
else if (BOOST_OS_UNIX != 0) { return PERFORMOUS_UNIX; }
}

uint16_t Platform::shortcutModifier(bool eitherSide) {
	if (currentOS() == PERFORMOUS_MAC) { return eitherSide ? KMOD_GUI : KMOD_LGUI; }
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
			case PERFORMOUS_WIN: return 13; // WASAPI
			case PERFORMOUS_MAC: return 5; // CoreAudio
			case PERFORMOUS_SOLARIS: return 7; // OSS
			case PERFORMOUS_BSD: return 7; // OSS
			case PERFORMOUS_LINUX: return 8; // ALSA
			case PERFORMOUS_UNIX: return 8; // ALSA
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