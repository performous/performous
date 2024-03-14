#include "common.hh"

#include "game/guitar/guitar_strings.hh"

TEST(UnitTest_GuitarStrings, getBaseFrequency) {
	EXPECT_EQ(82.41f, GuitarStrings().getBaseFrequency(StringName::E_Low));
	EXPECT_EQ(110.0f, GuitarStrings().getBaseFrequency(StringName::A));
	EXPECT_EQ(146.83f, GuitarStrings().getBaseFrequency(StringName::D));
	EXPECT_EQ(196.00f, GuitarStrings().getBaseFrequency(StringName::G));
	EXPECT_EQ(246.94f, GuitarStrings().getBaseFrequency(StringName::B));
	EXPECT_EQ(329.63f, GuitarStrings().getBaseFrequency(StringName::E_High));
}

TEST(UnitTest_GuitarStrings, getFrequency_fret_default) {
	EXPECT_EQ(82.41f, GuitarStrings().getFrequency(StringName::E_Low));
	EXPECT_EQ(110.0f, GuitarStrings().getFrequency(StringName::A));
	EXPECT_EQ(146.83f, GuitarStrings().getFrequency(StringName::D));
	EXPECT_EQ(196.00f, GuitarStrings().getFrequency(StringName::G));
	EXPECT_EQ(246.94f, GuitarStrings().getFrequency(StringName::B));
	EXPECT_EQ(329.63f, GuitarStrings().getFrequency(StringName::E_High));
}

TEST(UnitTest_GuitarStrings, getFrequency_fret_0) {
	EXPECT_EQ(82.41f, GuitarStrings().getFrequency(StringName::E_Low, 0));
	EXPECT_EQ(110.0f, GuitarStrings().getFrequency(StringName::A, 0));
	EXPECT_EQ(146.83f, GuitarStrings().getFrequency(StringName::D, 0));
	EXPECT_EQ(196.00f, GuitarStrings().getFrequency(StringName::G, 0));
	EXPECT_EQ(246.94f, GuitarStrings().getFrequency(StringName::B, 0));
	EXPECT_EQ(329.63f, GuitarStrings().getFrequency(StringName::E_High, 0));
}

TEST(UnitTest_GuitarStrings, getFrequency_fret_1) {
	EXPECT_NEAR(87.31f, GuitarStrings().getFrequency(StringName::E_Low, 1), 0.01f);
	EXPECT_NEAR(116.54f, GuitarStrings().getFrequency(StringName::A, 1), 0.01f);
	EXPECT_NEAR(155.56f, GuitarStrings().getFrequency(StringName::D, 1), 0.01f);
	EXPECT_NEAR(207.65f, GuitarStrings().getFrequency(StringName::G, 1), 0.01f);
	EXPECT_NEAR(261.63f, GuitarStrings().getFrequency(StringName::B, 1), 0.01f);
	EXPECT_NEAR(349.23f, GuitarStrings().getFrequency(StringName::E_High, 1), 0.01f);
}

TEST(UnitTest_GuitarStrings, getFrequency_fret_12) {
	EXPECT_NEAR(164.81f, GuitarStrings().getFrequency(StringName::E_Low, 12), 0.1f);
	EXPECT_NEAR(220.00f, GuitarStrings().getFrequency(StringName::A, 12), 0.1f);
	EXPECT_NEAR(293.66f, GuitarStrings().getFrequency(StringName::D, 12), 0.1f);
	EXPECT_NEAR(392.00f, GuitarStrings().getFrequency(StringName::G, 12), 0.1f);
	EXPECT_NEAR(493.88f, GuitarStrings().getFrequency(StringName::B, 12), 0.1f);
	EXPECT_NEAR(659.26f, GuitarStrings().getFrequency(StringName::E_High, 12), 0.1f);
}
