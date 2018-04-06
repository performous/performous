#pragma once

#include <cstdint>
#include <limits>
#include <vector>
#include <stdexcept>

constexpr double TAU = 2.0 * 3.141592653589793238462643383279502884;  // https://tauday.com/tau-manifesto

/** Templated conversion functions for string to T, specializations defined in util.cc **/
template <typename T> T sconv(std::string const& s);

/** Limit val to range [min, max] **/
template <typename T> constexpr T clamp(T val, T min = 0, T max = 1) {
	if (min > max) throw std::logic_error("min > max");
	if (val < min) return min;
	if (val > max) return max;
	return val;
}

template <typename Numeric> struct MinMax {
	Numeric min, max;
	explicit MinMax(Numeric min = std::numeric_limits<Numeric>::min(), Numeric max = std::numeric_limits<Numeric>::max()): min(min), max(max) {}
	bool matches(Numeric val) const { return val >= min && val <= max; }
};

/** A convenient way for getting NaNs **/
static inline constexpr double getNaN() { return std::numeric_limits<double>::quiet_NaN(); }

/** A convenient way for getting infs **/
static inline constexpr double getInf() { return std::numeric_limits<double>::infinity(); }

/** OpenGL smoothstep function **/
template <typename T> constexpr T smoothstep(T edge0, T edge1, T x) {
	x = clamp((x - edge0) / (edge1 - edge0));
	return x * x * (3 - 2 * x);
}

/** Convenience smoothstep wrapper with edges at 0 and 1 **/
template <typename T> constexpr T smoothstep(T x) { return smoothstep<T>(0, 1, x); }
