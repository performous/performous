#include "math.hh"

#include <cmath>
#include <algorithm>

float pi() {
#if defined( WIN32) || defined( __MINGW32__ )
	return static_cast<float>(std::atan(1.0) * 4.0);
#else
	return static_cast<float>(M_PI);
#endif
}

float clamp(float value, float min, float max)
{
	return std::max(min, std::min(max, value));
}

float mix(float valueA, float valueB, float a)
{
	return valueA * (1.0f - a) + valueB * a;
}
