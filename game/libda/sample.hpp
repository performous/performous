#pragma once

/**
 * @file sample.hpp Sample format definition and format conversions.
 */

namespace da {

	// Implement mathematical rounding (which C++ unfortunately currently lacks)
	template <typename T> T round(T val) { return static_cast<T>(static_cast<int>(val + (val >= 0 ? 0.5 : -0.5))); }

	// WARNING: changing this breaks binary compatibility on the library!
	typedef float sample_t;

	// A helper function for clamping a value to a certain range
	template <typename T> T clamp(T val, T min, T max) {
		if (val < min) val = min;
		if (val > max) val = max;
		return val;
	}

	const sample_t max_s16 = 32767.0f, min_s16 = -max_s16 - 1.0f;
	const sample_t max_s24 = 8388607.0f, min_s24 = -max_s24 - 1.0f;
	const sample_t max_s32 = 2147483647.0f, min_s32= -max_s32 - 1.0f;

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
