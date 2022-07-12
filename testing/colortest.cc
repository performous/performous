#include "game/color.hh"

#include "common.hh"

namespace {
	TEST(UnitTest_Color, default_ctor_white) {
		EXPECT_EQ(1.0f, Color().r);
		EXPECT_EQ(1.0f, Color().g);
		EXPECT_EQ(1.0f, Color().b);
		EXPECT_EQ(1.0f, Color().a);
	}
}

