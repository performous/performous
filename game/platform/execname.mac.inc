#include <mach-o/dyld.h>

fs::path execname() {
	char buf[1024];
	uint32_t size = sizeof(buf);
	int ret = _NSGetExecutablePath(buf, &size);
	if (ret != 0) return fs::path();
	return buf;
}