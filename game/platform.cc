#include "platform.hh"
#include "fs.hh"

Platform::Host_OS Platform::currentOS() {
if (BOOST_OS_WINDOWS != 0) { return Host_OS::Performous_OS_Win32; }
else if (BOOST_OS_LINUX != 0) { return Host_OS::Performous_OS_Linux; }
else if (BOOST_OS_MACOS != 0) { return Host_OS::Performous_OS_macOS; }
else if (BOOST_OS_BSD != 0) { return Host_OS::Performous_OS_BSD; }
else if (BOOST_OS_SOLARIS != 0) { return Host_OS::Performous_OS_Solaris; }
else if (BOOST_OS_UNIX != 0) { return Host_OS::Performous_OS_Unix; }
}

uint16_t Platform::shortcutModifier(bool eitherSide) {
	if (currentOS() == Host_OS::Performous_OS_macOS) { return eitherSide ? KMOD_GUI : KMOD_LGUI; }
	else { return eitherSide ? KMOD_CTRL : KMOD_LCTRL; }
}

Platform::Platform() {
	#if (BOOST_OS_WINDOWS)
	_putenv_s("FONTCONFIG_PATH",".\\etc\\");
	#endif
}

const std::array<const char*,6> Platform::platformNames = {{ "Windows", "MacOS", "BSD", "Solaris", "Linux", "Unix" }}; // Relevant for debug only.

int Platform::defaultBackEnd() {
		switch (Platform::currentOS()) {
			case Host_OS::Performous_OS_Win32: return 13; // WASAPI
			case Host_OS::Performous_OS_macOS: return 5; // CoreAudio
			case Host_OS::Performous_OS_Solaris: return 7; // OSS
			case Host_OS::Performous_OS_BSD: return 7; // OSS
			case Host_OS::Performous_OS_Linux: return 8; // ALSA
			case Host_OS::Performous_OS_Unix: return 8; // ALSA
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