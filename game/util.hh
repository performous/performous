#ifndef PERFORMOUS_UTIL_HH
#define PERFORMOUS_UTIL_HH

#include <limits>
#include <stdexcept>

/** Limit val to range [min, max] **/
template <typename T> T clamp(T val, T min = 0, T max = 1) {
	if (min > max) throw std::logic_error("min > max");
	if (val < min) return min;
	if (val > max) return max;
	return val;
}

/** A convenient way for getting NaNs **/
static inline double getNaN() { return std::numeric_limits<double>::quiet_NaN(); }

#endif

