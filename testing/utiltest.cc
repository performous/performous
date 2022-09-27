#include <gtest/gtest.h>

#include "game/util.hh"

namespace {
	TEST(UnitTest_Utils, clamp) {
		EXPECT_EQ(0.0, clamp(-1.0, 0.0, 1.0));
		EXPECT_EQ(0.5, clamp(0.5, 0.0, 1.0));
		EXPECT_EQ(1.0, clamp(2.0, 0.0, 1.0));
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

	TEST(UnitTest_Utils, format) {
		auto const time = std::chrono::seconds(7260);

		// next two tests are not working when compiled with mingw. see https://sourceforge.net/p/mingw-w64/bugs/793/
		//EXPECT_EQ("01/01/70 02:01", format(time, "%D %R"));
		//EXPECT_EQ("02:01:00 1970-01-01", format(time, "%T %F"));
		EXPECT_EQ("01/01/70 02:01", format(time, "%m/%d/%y %H:%M", true));
		EXPECT_EQ("02:01:00 1970-01-01", format(time, "%H:%M:%S %Y-%m-%d", true));
		EXPECT_EQ("02:01:00 1970-01-01", format(time, "%X %Y-%m-%d", true));
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

	TEST(UnitTest_Utils, erase_empty_empty) {
		EXPECT_EQ("", erase("", ""));
	}

	TEST(UnitTest_Utils, erase_empty_x) {
		EXPECT_EQ("", erase("", "x"));
	}

	TEST(UnitTest_Utils, erase_x_empty) {
		EXPECT_EQ("x", erase("x", ""));
	}

	TEST(UnitTest_Utils, erase_x_x) {
		EXPECT_EQ("", erase("x", "x"));
	}

	TEST(UnitTest_Utils, erase_xy_x) {
		EXPECT_EQ("y", erase("xy", "x"));
	}

	TEST(UnitTest_Utils, erase_yx_x) {
		EXPECT_EQ("y", erase("yx", "x"));
	}

	TEST(UnitTest_Utils, erase_yx_xy) {
		EXPECT_EQ("yx", erase("yx", "xy"));
	}

	TEST(UnitTest_Utils, erase_xyx_xy) {
		EXPECT_EQ("x", erase("xyx", "xy"));
	}

	TEST(UnitTest_Utils, replace_char_empty) {
		EXPECT_EQ("", replace("", 'a', 'b'));
	}

	TEST(UnitTest_Utils, replace_char_one) {
		EXPECT_EQ("bb", replace("ab", 'a', 'b'));
	}

	TEST(UnitTest_Utils, replace_string_empty) {
		EXPECT_EQ("", replace("", "a", "b"));
	}

	TEST(UnitTest_Utils, replace_string_one_by_one) {
		EXPECT_EQ("bb", replace("ab", "a", "b"));
	}

	TEST(UnitTest_Utils, replace_string_not_found) {
		EXPECT_EQ("ab", replace("ab", "c", "b"));
	}

	TEST(UnitTest_Utils, replace_string_by_empty) {
		EXPECT_EQ("b", replace("ab", "a", ""));
	}

	TEST(UnitTest_Utils, replace_string_one_by_two) {
		EXPECT_EQ("dcb", replace("ab", "a", "dc"));
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

	TEST(UnitTest_Utils, trimLeft_empty) {
		EXPECT_EQ("", trimLeft(""));
	}

	TEST(UnitTest_Utils, trimLeft_front) {
		EXPECT_EQ("A", trimLeft(" A"));
	}

	TEST(UnitTest_Utils, trimLeft_back) {
		EXPECT_EQ("A ", trimLeft("A "));
	}

	TEST(UnitTest_Utils, trimLeft_mid) {
		EXPECT_EQ("A B", trimLeft("A B"));
	}

	TEST(UnitTest_Utils, trimRight_empty) {
		EXPECT_EQ("", trimRight(""));
	}

	TEST(UnitTest_Utils, trimRight_front) {
		EXPECT_EQ(" A", trimRight(" A"));
	}

	TEST(UnitTest_Utils, trimRight_back) {
		EXPECT_EQ("A", trimRight("A "));
	}

	TEST(UnitTest_Utils, trimRight_mid) {
		EXPECT_EQ("A B", trimRight("A B"));
	}

	TEST(UnitTest_Utils, startsWith_empty_empty) {
		EXPECT_TRUE(startsWith("", ""));
	}

	TEST(UnitTest_Utils, startsWith_empty_x) {
		EXPECT_FALSE(startsWith("", "x"));
	}

	TEST(UnitTest_Utils, startsWith_x_empty) {
		EXPECT_TRUE(startsWith("x", ""));
	}

	TEST(UnitTest_Utils, startsWith_x_x) {
		EXPECT_TRUE(startsWith("x", "x"));
	}

	TEST(UnitTest_Utils, startsWith_xy_x) {
		EXPECT_TRUE(startsWith("xy", "x"));
	}

	TEST(UnitTest_Utils, startsWith_yx_x) {
		EXPECT_FALSE(startsWith("yx", "x"));
	}
}
