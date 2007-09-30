#ifndef LIBDA_SAMPLE_HPP_INCLUDED
#define LIBDA_SAMPLE_HPP_INCLUDED

/**
@file sample.hpp Sample format definition and format conversions.

Header-only, no need to link to LibDA.
**/

#include <math.h>

namespace da {
	// WARNING: changing this breaks binary compatibility on the library!
	typedef float sample_t;

	// The following conversions provide lossless conversions between floats
	// and integers. Be sure to use only these conversions or otherwise the
	// conversions may not be lossless, due to different scaling factors being
	// used by different libraries.

	// The negative minimum integer value produces sample_t value slightly
	// more negative than -1.0 but this is necessary in order to prevent
	// clipping in the float-to-int conversions. Now amplitude 1.0 in floating
	// point produces -32767 .. 32767 symmetrical non-clipping range in s16.
	const sample_t max_s16 = 32767.0;
	const sample_t max_s24 = 8388607.0;

	static inline sample_t conv_from_s16(int s) { return s / max_s16; }
	static inline sample_t conv_from_s24(int s) { return s / max_s24; }
	// The rounding is strictly not necessary, but it greatly improves
	// the error tolerance if any floating point calculations are done.
	static inline int conv_to_s16(sample_t s) { return int(roundf(s * max_s16)); }
	static inline int conv_to_s24(sample_t s) { return int(roundf(s * max_s24)); }
}

#endif

