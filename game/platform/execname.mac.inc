#include <mach-o/dyld.h>
#include <cstdint>

fs::path execname() {
	char buf[1024];
	std::uint32_t size = sizeof(buf);
	int ret = _NSGetExecutablePath(buf, &size);
	if (ret != 0) return fs::path();
	return buf;
}