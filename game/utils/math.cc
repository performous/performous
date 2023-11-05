#include "math.hh"

#include <cmath>

float pi() {
#if defined( WIN32) || defined( __MINGW32__ )
	return static_cast<float>(std::atan(1.0) * 4.0);
#else
	return static_cast<float>(M_PI);
#endif
}
