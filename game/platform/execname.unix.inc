#include <unistd.h>

fs::path execname() {
	char buf[1024];
	ssize_t maxchars = sizeof(buf) - 1;
	ssize_t size = readlink("/proc/self/exe", buf, sizeof(buf));
	if (size <= 0 || size >= maxchars) return fs::path();
	buf[size] = '\0';
	return buf;
}