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

		EXPECT_EQ("01/01/70 02:01", format(time, "%D %R"));
		EXPECT_EQ("02:01:00 1970-01-01", format(time, "%T %F"));
	}
}
