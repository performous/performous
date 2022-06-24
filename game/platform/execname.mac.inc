#include <mach-o/dyld.h>
#include <array>
#include <cstdint>

fs::path execname() {
	std::array<char, 1024> buf;
	std::uint32_t size = buf.size();
	int ret = _NSGetExecutablePath(buf.data(), &size);
	if (ret != 0) return fs::path();
	return buf.data();
}