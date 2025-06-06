#include "common.hh"

#include "game/util.hh"

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

