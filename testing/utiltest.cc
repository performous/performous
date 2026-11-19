#include "common.hh"

#include "game/util.hh"

#include <cfenv>
#include <sstream>

TEST(UnitTest_Utils, clamp) {
	EXPECT_EQ(0.0, clamp(-1.0, 0.0, 1.0));
	EXPECT_EQ(0.5, clamp(0.5, 0.0, 1.0));
	EXPECT_EQ(1.0, clamp(2.0, 0.0, 1.0));
}

namespace {
	template <typename T>
	T stepUlps(T start, T direction, int ulps) {
		T val = start;
		for (int i = 0; i < ulps; ++i) {
			val = std::nextafter(val, direction);
		}
		return val;
	}

/// almostEqual helper function for tests. In case of a failed test, it will prints operands and epsilon with 30 digits of preicision.
	template <typename T>
	::testing::AssertionResult CheckAlmostEqual(T lhs, T rhs, bool expected, std::remove_cv_t<T> abs_tol = defaultTolerance<T>()) {
		const bool actual = almostEqual<T>(lhs, rhs, abs_tol);
		if (actual == expected) {
			return ::testing::AssertionSuccess();
		}

		std::ostringstream oss;
		oss << std::boolalpha
			<< "almostEqual(lhs, rhs, abs_tol) returned " << actual
			<< ", expected " << expected << "\n"
			<< std::fixed << std::setprecision(30)
			<< "  lhs:	   " << lhs << "\n"
			<< "  rhs:	   " << rhs << "\n"
			<< "  abs_tol: " << abs_tol;

		return ::testing::AssertionFailure() << oss.str();
	}

} // namespace

TEST(UnitTest_Utils, almostEqual_exact) {
	EXPECT_TRUE(CheckAlmostEqual(1.0, 1.0, true));
}

TEST(UnitTest_Utils, almostEqual_within_default_float_epsilon) {
	EXPECT_TRUE(CheckAlmostEqual(1.0f, 1.00000010f, true));
}

TEST(UnitTest_Utils, almostEqual_within_default_double_epsilon) {
	EXPECT_TRUE(CheckAlmostEqual(1.0, 1.00000000000000014, true));
}

TEST(UnitTest_Utils, almostEqual_outside_default_float_epsilon) {
	EXPECT_TRUE(CheckAlmostEqual(1.0f, 1.00000056f, false));
}

TEST(UnitTest_Utils, almostEqual_outside_default_double_epsilon) {
	EXPECT_TRUE(CheckAlmostEqual(1.0, 1.0000000000000011, false));
}

TEST(UnitTest_Utils, almostEqual_custom_epsilon) {
	EXPECT_TRUE(CheckAlmostEqual(1.0, 1.05, true, 0.1));
	EXPECT_TRUE(CheckAlmostEqual(1.0, 1.2, false, 0.1));
}


TEST(UnitTest_Utils, almostEqual_multi_step_arithmetic_and_binade_boundaries) {
	const float sum_1_7 = 1.1f + 0.1f + 0.1f + 0.1f + 0.1f + 0.2f - 0.1f + 0.1f * 1.0f;
	EXPECT_TRUE(CheckAlmostEqual(sum_1_7, 1.7f, true))
		<< "1a. Accumulated float chain to 1.7f (1 ULP drift)";

	const double sum_1_4 = 1.1 + 0.1 + 0.1 + 0.1;
	EXPECT_TRUE(CheckAlmostEqual(sum_1_4, 1.4, true))
		<< "1b. Accumulated double addition to 1.4 (2 ULPs drift)";

	const double sub_amp = 10.0 * (1.1 - 1.0);
	EXPECT_TRUE(CheckAlmostEqual(sub_amp, 1.0, true))
		<< "1c. Catastrophic cancellation amplification 10*(1.1 - 1.0) vs 1.0 (4 ULPs)";

	// Bottom of binade [1.0f, 2.0f): 1 ULP == eps -> 4 ULPs pass, 5 ULPs fail
	const float above_1_4ulp = stepUlps(1.0f, 2.0f, 4);
	const float above_1_5ulp = stepUlps(1.0f, 2.0f, 5);
	EXPECT_TRUE(CheckAlmostEqual(1.0f, above_1_4ulp, true))
		<< "1d. Bottom of binade: 4 ULPs above 1.0f";
	EXPECT_TRUE(CheckAlmostEqual(1.0f, above_1_5ulp, false))
		<< "1e. Bottom of binade: 5 ULPs above 1.0f";

	// Top of binade [1.0f, 2.0f): 1 ULP == 0.5 * eps relative to 2.0f -> 8 ULPs pass, 9 ULPs fail
	const float below_2_8ulp = stepUlps(2.0f, 1.0f, 8);
	const float below_2_9ulp = stepUlps(2.0f, 1.0f, 9);
	EXPECT_TRUE(CheckAlmostEqual(2.0f, below_2_8ulp, true))
		<< "1f. Top of binade: 8 ULPs below 2.0f";
	EXPECT_TRUE(CheckAlmostEqual(2.0f, below_2_9ulp, false))
		<< "1g. Top of binade: 9 ULPs below 2.0f";
}

TEST(UnitTest_Utils, almostEqual_sizes_in_thousands) {
	// Summing 11 UI columns across a 4K (3840 px) canvas accumulates 2 ULPs of error
	const float col_w = 3840.0f / 11.0f;
	float canvas_w = 0.0f;
	for (int i = 0; i < 11; ++i) canvas_w += col_w;
	EXPECT_TRUE(CheckAlmostEqual(canvas_w, 3840.0f, true))
		<< "2a. 4K (3840.0f) layout: 11 accumulated column widths vs 3840.0f";

	// 5K (5120.0f): 3-ULP transform drift passes, 0.01f sub-pixel offset fails
	const float coord_5k_3ulp = stepUlps(5120.0f, 10000.0f, 3);
	const float coord_5k_shift = 5120.01f;
	EXPECT_TRUE(CheckAlmostEqual(5120.0f, coord_5k_3ulp, true))
		<< "2b. 5K (5120.0f) coordinate with 3-ULP transform drift";
	EXPECT_TRUE(CheckAlmostEqual(5120.0f, coord_5k_shift, false))
		<< "2c. 5K (5120.0f) coordinate with genuine 0.01f sub-pixel shift";

	// Custom abs_tol is never scaled by `largest`
	EXPECT_TRUE(CheckAlmostEqual(1680.0f, 840.0f, false, 1.0f))
		<< "2d. Large screen coords with custom 1.0f tol (1680.0f vs 840.0f)";
	EXPECT_TRUE(CheckAlmostEqual(3840.0f, 3840.75f, true, 1.0f))
		<< "2e. 4K coordinate within 1.0f custom pixel-snap tolerance";
	EXPECT_TRUE(CheckAlmostEqual(3840.0f, 3841.25f, false, 1.0f))
		<< "2f. 4K coordinate outside 1.0f custom pixel-snap tolerance";
}

TEST(UnitTest_Utils, almostEqual_time_and_animations_double) {
	// 60 FPS delta-time accumulation over 12 frames (0.2s)
	const double dt_60 = 1.0 / 60.0;
	double t_accum = 0.0;
	for (int i = 0; i < 12; ++i) t_accum += dt_60;
	EXPECT_TRUE(CheckAlmostEqual(t_accum, 0.2, true))
		<< "3a. 60 FPS frame accumulation (12 * 1/60s vs 0.2s)";

	// Long-running timer at 1 hour (3600.0s) advancing by 11 sub-steps of 1/11s to 3601.0s
	double timer_1h = 3600.0;
	const double step_11 = 1.0 / 11.0;
	for (int i = 0; i < 11; ++i) timer_1h += step_11;
	EXPECT_TRUE(CheckAlmostEqual(timer_1h, 3601.0, true))
		<< "3b. Long-running timer at 3600s + 11*(1/11s) vs 3601.0s";
	EXPECT_TRUE(CheckAlmostEqual(3600.0, 3600.0 + 1e-9, false))
		<< "3c. Long-running timer at 3600.0s vs 1-nanosecond offset";
}

TEST(UnitTest_Utils, almostEqual_zero_crossings_and_controller_deadzones) {
	constexpr float tol_f = defaultTolerance<float>();

	EXPECT_TRUE(CheckAlmostEqual(-0.0f, +0.0f, true))
		<< "4a. Signed zeros (-0.0f vs +0.0f)";
	EXPECT_TRUE(CheckAlmostEqual(tol_f, -tol_f, false))
		<< "4b. Opposite-sign error doubling guard (+tol vs -tol, distance = 2*tol)";
	EXPECT_TRUE(CheckAlmostEqual(0.45f * tol_f, -0.45f * tol_f, true))
		<< "4c. Symmetric zero-crossing within floor (+0.45*tol vs -0.45*tol)";
	EXPECT_TRUE(CheckAlmostEqual(0.55f * tol_f, -0.55f * tol_f, false))
		<< "4d. Symmetric zero-crossing outside floor (+0.55*tol vs -0.55*tol)";

	// Controller stick deadzone across 0.0f with custom abs_tol = 0.025f
	EXPECT_TRUE(CheckAlmostEqual(-0.012f, 0.011f, true, 0.025f))
		<< "4e. Controller axis across zero within 0.025f deadzone";
	EXPECT_TRUE(CheckAlmostEqual(-0.015f, 0.015f, false, 0.025f))
		<< "4f. Controller axis across zero outside 0.025f deadzone";
}

TEST(UnitTest_Utils, almostEqual_IEEE754_extremes_overflow_and_cv_qualifiers) {
	// Opposite-sign max values must not overflow or raise FE_OVERFLOW
	std::feclearexcept(FE_ALL_EXCEPT);
	volatile float max_f = std::numeric_limits<float>::max();
	const float pos_max = max_f;
	const float neg_max = -max_f;
	const bool max_cmp = almostEqual<float>(pos_max, neg_max);
	const bool fe_overflow = (std::fetestexcept(FE_OVERFLOW) != 0);

	EXPECT_FALSE(max_cmp) << "5a. Opposite-sign FLT_MAX vs -FLT_MAX returns false";
	EXPECT_FALSE(fe_overflow) << "5b. Opposite-sign FLT_MAX vs -FLT_MAX does NOT raise FE_OVERFLOW";

	// NaNs, Infinities, negative tolerance, and CV-qualified instantiation
	constexpr float nan_f = std::numeric_limits<float>::quiet_NaN();
	constexpr float inf_f = std::numeric_limits<float>::infinity();

	EXPECT_TRUE(CheckAlmostEqual(nan_f, nan_f, false))
		<< "5c. NaN vs NaN";
	EXPECT_TRUE(CheckAlmostEqual(inf_f, -inf_f, false))
		<< "5d. +Infinity vs -Infinity";
	EXPECT_TRUE(CheckAlmostEqual(inf_f, pos_max, false))
		<< "5e. +Infinity vs FLT_MAX";
	EXPECT_TRUE(CheckAlmostEqual(1.0f, stepUlps(1.0f, 2.0f, 1), false, -0.01f))
		<< "5f. Negative custom abs_tol is rejected";

	const float lhs_const = 1920.0f;
	const float rhs_const = stepUlps(1920.0f, 4000.0f, 3);
	EXPECT_TRUE(CheckAlmostEqual<const float>(lhs_const, rhs_const, true))
		<< "5g. Explicit <const float> on 3-ULP adjacent values at 1920.0f";
}

TEST(UnitTest_Utils, smoothstep_1) {
	EXPECT_EQ(0.0, smoothstep(0));
	EXPECT_EQ(0.5, smoothstep(0.5));
	EXPECT_EQ(1.0, smoothstep(1.0));
}

TEST(UnitTest_Utils, smoothstep_1_clamping) {
	EXPECT_EQ(0.0, smoothstep(-1.0));
	EXPECT_EQ(1.0, smoothstep(2.0));
}

TEST(UnitTest_Utils, smoothstep_3) {
	EXPECT_EQ(0.0, smoothstep(-1., 1., -1.0));
	EXPECT_EQ(0.5, smoothstep(-1., 1., 0.0));
	EXPECT_EQ(1.0, smoothstep(-1., 1., 1.0));
}

TEST(UnitTest_Utils, smoothstep_3_clamping) {
	EXPECT_EQ(0.0, smoothstep(-1., 1., -2.0));
	EXPECT_EQ(1.0, smoothstep(-1., 1., 2.0));
}

TEST(UnitTest_Utils, timeFormat) {
	auto const time = std::chrono::seconds(7260);

#ifndef __MINGW32__
	// next two tests are not working when compiled with mingw. see https://sourceforge.net/p/mingw-w64/bugs/793/
	EXPECT_EQ("01/01/70 02:01", timeFormat(time, "%D %R", true));
	EXPECT_EQ("02:01:00 1970-01-01", timeFormat(time, "%T %F", true));
#endif

	EXPECT_EQ("01/01/70 02:01", timeFormat(time, "%m/%d/%y %H:%M", true));
	EXPECT_EQ("02:01:00 1970-01-01", timeFormat(time, "%H:%M:%S %Y-%m-%d", true));
	EXPECT_EQ("02:01:00 1970-01-01", timeFormat(time, "%X %Y-%m-%d", true));
}

	TEST(UnitTest_Utils, toLower_empty) {
		EXPECT_EQ("", toLower(""));
	}

	TEST(UnitTest_Utils, toLower_lower_case) {
		EXPECT_EQ("lower", toLower("lower"));
	}

	TEST(UnitTest_Utils, toLower_upper_case) {
		EXPECT_EQ("upper", toLower("UPPER"));
	}

	TEST(UnitTest_Utils, toLower_mixed_case) {
		EXPECT_EQ("mixed", toLower("MiXed"));
	}

	TEST(UnitTest_Utils, toLower_digits) {
		EXPECT_EQ("0123", toLower("0123"));
	}

	TEST(UnitTest_Utils, toLower_umlaute) {
		// no "umlaute" support
		EXPECT_NE("äöüäöü", toLower("äöüÄÖÜ"));
	}

	TEST(UnitTest_Utils, toUpper_empty) {
		EXPECT_EQ("", toUpper(""));
	}

	TEST(UnitTest_Utils, toUpper_lower_case) {
		EXPECT_EQ("LOWER", toUpper("lower"));
	}

	TEST(UnitTest_Utils, toUpper_upper_case) {
		EXPECT_EQ("UPPER", toUpper("UPPER"));
	}

	TEST(UnitTest_Utils, toUpper_mixed_case) {
		EXPECT_EQ("MIXED", toUpper("MiXed"));
	}

	TEST(UnitTest_Utils, toUpper_digits) {
		EXPECT_EQ("0123", toUpper("0123"));
	}

	TEST(UnitTest_Utils, toUpper_umlaute) {
		// no "umlaute" support
		EXPECT_NE("äöüäöü", toUpper("äöüÄÖÜ"));
	}

	TEST(UnitTest_Utils, replace_empty) {
		EXPECT_EQ("", replace("", 'a', 'b'));
	}

	TEST(UnitTest_Utils, replace_one) {
		EXPECT_EQ("bb", replace("ab", 'a', 'b'));
	}

	TEST(UnitTest_Utils, trim_empty) {
		EXPECT_EQ("", trim(""));
	}

	TEST(UnitTest_Utils, trim_one_space) {
		EXPECT_EQ("", trim(" "));
	}

	TEST(UnitTest_Utils, trim_two_spaces) {
		EXPECT_EQ("", trim("  "));
	}

	TEST(UnitTest_Utils, trim_white_spaces) {
		EXPECT_EQ("", trim(" \n\r\t"));
	}

	TEST(UnitTest_Utils, trim_front_one_space) {
		EXPECT_EQ("X", trim(" X"));
	}

	TEST(UnitTest_Utils, trim_front_two_spaces) {
		EXPECT_EQ("X", trim("  X"));
	}

	TEST(UnitTest_Utils, trim_front_white_spaces) {
		EXPECT_EQ("X", trim(" \n\r\tX"));
	}

	TEST(UnitTest_Utils, trim_back_one_space) {
		EXPECT_EQ("X", trim("X "));
	}

	TEST(UnitTest_Utils, trim_back_two_spaces) {
		EXPECT_EQ("X", trim("X  "));
	}

	TEST(UnitTest_Utils, trim_back_white_spaces) {
		EXPECT_EQ("X", trim("X \n\r\t"));
	}

	TEST(UnitTest_Utils, trim_both_one_space) {
		EXPECT_EQ("X", trim(" X "));
	}

	TEST(UnitTest_Utils, trim_both_two_spaces) {
		EXPECT_EQ("X", trim("  X  "));
	}

	TEST(UnitTest_Utils, trim_both_white_spaces) {
		EXPECT_EQ("X", trim(" \n\r\tX \n\r\t"));
	}

	TEST(UnitTest_Utils, trim_mid_one_space) {
		EXPECT_EQ("X Y", trim(" X Y "));
	}

	TEST(UnitTest_Utils, trim_mid_two_spaces) {
		EXPECT_EQ("X  Y", trim("  X  Y  "));
	}

	TEST(UnitTest_Utils, trim_mid_white_spaces) {
		EXPECT_EQ("X \n\r\tY", trim(" \n\r\tX \n\r\tY \n\r\t"));
	}

TEST(UnitTest_Utils, make_iterator_range) {
	auto const v = std::vector<int>{ 0, 2, 4, 3, 1 };
	auto const begin = v.begin();
	auto const end = v.end();
	auto const range = make_iterator_range(begin, end);

	auto it = v.begin();

	for (auto const i : range)
		EXPECT_EQ(*it++, i);
}

TEST(UnitTest_Utils, reverse) {
	auto const v = std::vector<int>{ 0, 2, 4, 3, 1 };
	auto it = v.rbegin();

	for (auto const i : reverse(v))
		EXPECT_EQ(*it++, i);
}

TEST(UnitTest_Utils, isText_text) {
	EXPECT_TRUE(isText("text"));
}

TEST(UnitTest_Utils, isText_text_and_binary) {
	auto const binary = std::string{ char(0x01), char(0x02), char(0x03) };
	EXPECT_FALSE(isText("text" + binary + "other"));
}

TEST(UnitTest_Utils, isText_bom_and_text) {
	auto const bom = std::string{ char(0xEF), char(0xBB), char(0xBF) };

	EXPECT_TRUE(isText(bom + "text"));
}

TEST(UnitTest_Utils, isText_bom_and_text_and_binary) {
	auto const bom = std::string{ char(0xEF), char(0xBB), char(0xBF) };

	EXPECT_TRUE(isText(bom + "text\0other"));
}

TEST(UnitTest_Utils, isText_binary_zeros) {
	auto const binary = std::string{ char(0x00), char(0x00), char(0x00), char(0x00) };

	EXPECT_FALSE(isText(binary));
}

TEST(UnitTest_Utils, isText_binary_ffs) {
	auto const binary = std::string{ char(0xff), char(0xff), char(0xff), char(0xff) };

	EXPECT_FALSE(isText(binary));
}

TEST(UnitTest_Utils, isText_binary) {
	auto const binary = std::string{ char(0x01), char(0x02), char(0x03) };

	EXPECT_FALSE(isText(binary));
}

TEST(UnitTest_Utils, isText_with_nl) {
	EXPECT_TRUE(isText("first\nsecond"));
}

TEST(UnitTest_Utils, isText_with_lf) {
	EXPECT_TRUE(isText("first\rsecond"));
}

TEST(UnitTest_Utils, isText_with_tab) {
	EXPECT_TRUE(isText("first\tsecond"));
}

TEST(UnitTest_Utils, isText_utf8_umla) {
	auto const auml_utf8 = std::string{ char(0xC3), char(0xA4) };
	EXPECT_TRUE(isText("German auml: " + auml_utf8));
}

TEST(UnitTest_Utils, isText_utf8_euro) {
	auto const euro_utf8 = std::string{ char(0xE2), char(0x82), char(0xAC) };
	EXPECT_TRUE(isText("euro sign: " + euro_utf8));
}

TEST(UnitTest_Utils, isText_utf8_euro_wrong_follow_byte) {
	auto const euro_utf8 = std::string{ char(0xE2), char(0x82), char(0xAC), char(0xAC), char(0xAC) };
	EXPECT_FALSE(isText("euro sign: " + euro_utf8));
}

TEST(UnitTest_Utils, isText_utf8_4_byte_f09284a0) {
	auto const utf8 = std::string{ char(0xF0), char(0x92), char(0x84), char(0xA0) };
	EXPECT_TRUE(isText(utf8));
}

TEST(UnitTest_Utils, isText_utf8_4_byte_f0908d88) {
	auto const utf8 = std::string{ char(0xF0), char(0x90), char(0x8D), char(0x88) };
	EXPECT_TRUE(isText(utf8));
}

TEST(UnitTest_Utils, isText_text_utf8_4_byte) {
	auto const utf8 = std::string{ char(0xF0), char(0x90), char(0x8D), char(0x88) };
	EXPECT_TRUE(isText("4 byte utf-8: " + utf8));
}

TEST(UnitTest_Utils, isText_text_utf8_4_byte_text) {
	auto const utf8 = std::string{ char(0xF0), char(0x90), char(0x8D), char(0x88) };
	EXPECT_TRUE(isText("4 byte utf-8 " + utf8 + " inside ascii"));
}

TEST(UnitTest_Utils, isText_utf8_euro_border_1) {
	auto const euro_utf8 = std::string{ char(0xE2), char(0x82), char(0xAC) };
	EXPECT_TRUE(isText("euro sign: " + euro_utf8, 12));
}

TEST(UnitTest_Utils, isText_utf8_euro_border_2) {
	auto const euro_utf8 = std::string{ char(0xE2), char(0x82), char(0xAC) };
	EXPECT_TRUE(isText("euro sign: " + euro_utf8, 13));
}


namespace {
	struct CommaDecimalPoint : std::numpunct<char> {
		char do_decimal_point() const override { return ','; }
	};
}

TEST(UnitTest_Utils, sconv_float) {
	EXPECT_FLOAT_EQ(0.1f, sconv<float>("0.1"));
	EXPECT_FLOAT_EQ(1.5f, sconv<float>("1.500000"));
	EXPECT_DOUBLE_EQ(-2.25, sconv<double>("-2.25"));
}

TEST(UnitTest_Utils, sconv_float_accepts_comma_as_decimal_separator) {
	EXPECT_FLOAT_EQ(1.5f, sconv<float>("1,500000"));
	EXPECT_DOUBLE_EQ(120.5, sconv<double>("120,5"));
}

TEST(UnitTest_Utils, sconv_float_invalid_throws) {
	EXPECT_THROW(sconv<float>(""), std::invalid_argument);
	EXPECT_THROW(sconv<float>("abc"), std::invalid_argument);
	EXPECT_THROW(sconv<double>("nan"), std::invalid_argument);
}

TEST(UnitTest_Utils, sconv_float_ignores_global_locale) {
	auto const previous = std::locale::global(std::locale(std::locale::classic(), new CommaDecimalPoint));
	EXPECT_FLOAT_EQ(0.1f, sconv<float>("0.1"));
	EXPECT_DOUBLE_EQ(1.5, sconv<double>("1.5"));
	std::locale::global(previous);
}
