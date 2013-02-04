#pragma once

#include <cmath>

/**
 * @file sample.hpp Sample format definition and format conversions.
 */

namespace da {

	using std::round;
	
	// Should be a floating-point type
	typedef float sample_t;

	// A helper function for clamping a value to a certain range
	template <typename T> T clamp(T val, T min, T max) {
		if (val < min) val = min;
		if (val > max) val = max;
		return val;
	}

	constexpr sample_t max_s16(32767), min_s16 = -max_s16 - sample_t(1);
	constexpr sample_t max_s24(8388607), min_s24 = -max_s24 - sample_t(1);
	constexpr sample_t max_s32(2147483647), min_s32= -max_s32 - sample_t(1);
	constexpr double tau(6.2831853071795864769252867665590057683943387987502116);  ///< One full circle
	constexpr double pi = tau / sample_t(2);  ///< Half circle
	constexpr double eps(1e-10);  ///< Tiny positive value
	
	/// Mathematical sinc function
	template<typename Float> static inline Float msinc(Float x) { return std::abs(x) < Float(eps) ? Float(1) : std::sin(x) / x; }
	
	/// Normalized (signal processing) sinc function
	template<typename Float> static inline Float sinc(Float x) { return msinc(Float(pi) * x); }
	
	/// Lanczos kernel of size A
	template<unsigned A, typename Float> static inline Float lanc(Float x) {
		return std::abs(x) < A ? sinc(x) * sinc(x / A) : Float();
	}

	// The following conversions provide lossless conversions between floats
	// and integers. Be sure to use only these conversions or otherwise the
	// conversions may not be lossless, due to different scaling factors being
	// used by different libraries.

	// The negative minimum integer value produces sample_t value slightly
	// more negative than -1.0 but this is necessary in order to prevent
	// clipping in the float-to-int conversions. Now amplitude 1.0 in floating
	// point produces -32767 .. 32767 symmetrical non-clipping range in s16.

	static inline sample_t conv_from_s16(int s) { return s / max_s16; }
	static inline sample_t conv_from_s24(int s) { return s / max_s24; }
	static inline sample_t conv_from_s32(int s) { return s / max_s32; }
	// The rounding is strictly not necessary, but it greatly improves
	// the error tolerance if any floating point calculations are done.
	// The ugly static_casts are required to avoid warnings in MSVC.
	static inline int conv_to_s16(sample_t s) { return clamp(static_cast<int>(round(s * max_s16)), static_cast<int>(min_s16), static_cast<int>(max_s16)); }
	static inline int conv_to_s24(sample_t s) { return clamp(static_cast<int>(round(s * max_s24)), static_cast<int>(min_s24), static_cast<int>(max_s24)); }
	static inline int conv_to_s32(sample_t s) { return static_cast<int>(clamp(round(s * max_s32), min_s32, max_s32 )); }
	// Non-rounding non-clamping versions are provided for very low end devices (still lossless)
	static inline int conv_to_s16_fast(sample_t s) { return static_cast<int>(s * max_s16); }
	static inline int conv_to_s24_fast(sample_t s) { return static_cast<int>(s * max_s24); }
	static inline int conv_to_s32_fast(sample_t s) { return static_cast<int>(s * max_s32); }

	template <typename ValueType> class step_iterator: public std::iterator<std::random_access_iterator_tag, ValueType> {
		ValueType* m_pos;
		std::ptrdiff_t m_step;
	  public:
		step_iterator(ValueType* pos, std::ptrdiff_t step): m_pos(pos), m_step(step) {}
		ValueType& operator*() { return *m_pos; }
		step_iterator operator+(std::ptrdiff_t rhs) { return step_iterator(m_pos + m_step * rhs, m_step); }
		step_iterator& operator++() { m_pos += m_step; return *this; }
		step_iterator operator++(int) { step_iterator ret = *this; ++*this; return ret; }
		bool operator!=(step_iterator const& rhs) const { return m_pos != rhs.m_pos; }
		std::ptrdiff_t operator-(step_iterator const& rhs) const { return (m_pos - rhs.m_pos) / m_step; }
		// TODO: more operators
	};

	typedef step_iterator<sample_t> sample_iterator;
	typedef step_iterator<sample_t const> sample_const_iterator;
}
