#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

fs::path execname() {
	char buf[1024];
	DWORD ret = GetModuleFileName(NULL, buf, sizeof(buf));
	if (ret == 0 || ret == sizeof(buf)) return std::string();
	return buf;
}