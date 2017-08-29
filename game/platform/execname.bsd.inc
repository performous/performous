#include <sys/sysctl.h>

fs::path execname() {
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	char buf[1024];
	size_t maxchars = sizeof(buf) - 1;
	size_t size = maxchars;
	sysctl(mib, 4, buf, &size, NULL, 0);
	if (size == 0 || size >= maxchars) return fs::path();
	buf[size] = '\0';
	return buf;
}