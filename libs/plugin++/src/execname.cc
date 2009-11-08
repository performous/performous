#include <plugin++/execname.hpp>

#if defined(_WIN32)
#include <windows.h>
std::string plugin::execname() {
	char buf[1024];
	DWORD ret = GetModuleFileName(NULL, buf, sizeof(buf));
	if (ret == 0 || ret == sizeof(buf)) return std::string();
	return buf;
}
#elif defined(__APPLE__)  // Not tested
#include <mach-o/dyld.h>
std::string plugin::execname() {
	char buf[1024];
	uint32_t size = sizeof(buf);
	int ret = _NSGetExecutablePath(buf, &size);
	if (ret != 0) return std::string();
	return buf;
}
#elif defined(sun) || defined(__sun)
#include <stdlib.h>
std::string plugin::execname() {
	return getplugin::execname();
}
#elif defined(__FreeBSD__)
#include <sys/sysctl.h>
std::string plugin::execname() {
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	char buf[1024];
	size_t size = sizeof(buf);
	sysctl(mib, 4, buf, &size, NULL, 0);
	if (size == 0 || size == sizeof(buf)) return std::string();
	return std::string(buf, size);
}
#elif defined(__linux__)
#include <unistd.h>
std::string plugin::execname() {
	char buf[1024];
	ssize_t ret = readlink("/proc/self/exe", buf, sizeof(buf));
	if (ret == 0 || ret == sizeof(buf)) return std::string();
	return std::string(buf, ret);
}
#else
std::string plugin::execname() {
	return std::string();
}
#endif

