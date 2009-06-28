#ifndef LIBDA_SAMPLE_HPP_INCLUDED
#define LIBDA_SAMPLE_HPP_INCLUDED

/**
@file sample.hpp Sample format definition and format conversions.

Header-only, no need to link to LibDA.
**/

namespace da {

	// Implement mathematical rounding (which C++ unfortunately currently lacks)
	template <typename T> T round(T val) { return int(val + (val >= 0 ? 0.5 : -0.5)); }
	
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
	static inline int conv_to_s16(sample_t s) { return clamp<int>(round(s * max_s16), min_s16, max_s16); }
	static inline int conv_to_s24(sample_t s) { return clamp<int>(round(s * max_s24), min_s24, max_s24); }
	static inline int conv_to_s32(sample_t s) { return int(clamp<sample_t>(round(s * max_s32), min_s32, max_s32 )); }
	// Non-rounding non-clamping versions are provided for very low end devices (still lossless)
	static inline int conv_to_s16_fast(sample_t s) { return int(s * max_s16); }
	static inline int conv_to_s24_fast(sample_t s) { return int(s * max_s24); }
	static inline int conv_to_s32_fast(sample_t s) { return int(s * max_s32); }
}

#endif

