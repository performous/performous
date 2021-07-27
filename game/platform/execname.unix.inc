#include <unistd.h>

fs::path execname() {
	char buf[1024];
	ssize_t size = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (size < 0) return fs::path();
	buf[size] = '\0';
	return buf;
}
