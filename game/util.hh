#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

constexpr double TAU = 2.0 * 3.141592653589793238462643383279502884;  // https://tauday.com/tau-manifesto

/** Templated conversion functions for string to T, specializations defined in util.cc **/
template <typename T> T sconv(std::string const& s);

/** Limit val to range [min, max] **/
template <typename T> constexpr T clamp(T val, T min = 0, T max = 1) {
	return (val < min) ? min : (val > max) ? max : val;
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
template <typename T> T smoothstep(T edge0, T edge1, T x) {
	x = clamp((x - edge0) / (edge1 - edge0));
	return x * x * (3 - 2 * x);
}

/** Convenience smoothstep wrapper with edges at 0 and 1 **/
template <typename T> T smoothstep(T x) { return smoothstep<T>(0, 1, x); }

/** Symetric of lock_guard: release a lock in constructor and take it back in destructor */
template <class Lockable>
struct UnlockGuard {
	UnlockGuard(Lockable& m) : m_mutex(m) { m_mutex.unlock(); }
	~UnlockGuard() { m_mutex.lock(); }

	UnlockGuard(const UnlockGuard&) = delete;
	UnlockGuard& operator=(const UnlockGuard&) = delete;

	private:
	Lockable& m_mutex;
};

