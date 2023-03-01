#include "game/utils/cycle.hh"

#include "common.hh"

TEST(UnitTest_Cycle, default_ctor_short) {
	EXPECT_EQ(0, Cycle<short>().get());
	EXPECT_EQ(0, Cycle<short>().getMin());
	EXPECT_EQ(1, Cycle<short>().getMax());
}

TEST(UnitTest_Cycle, default_ctor_unsigned_short) {
	EXPECT_EQ(0, Cycle<unsigned short>().get());
	EXPECT_EQ(0, Cycle<unsigned short>().getMin());
	EXPECT_EQ(1, Cycle<unsigned short>().getMax());
}

TEST(UnitTest_Cycle, ctor_unsigned_short) {
	EXPECT_EQ(2, Cycle<unsigned short>(2, 4, 1).get());
	EXPECT_EQ(1, Cycle<unsigned short>(2, 4, 1).getMin());
	EXPECT_EQ(4, Cycle<unsigned short>(2, 4, 1).getMax());
}

TEST(UnitTest_Cycle, ctor_short_bounds) {
	EXPECT_THROW(Cycle<short>(2, 1, -1), std::logic_error);
	EXPECT_NO_THROW(Cycle<short>(1, 1, -1));
	EXPECT_NO_THROW(Cycle<short>(-1, 1, -1));
	EXPECT_THROW(Cycle<short>(-2, 1, -1), std::logic_error);
}

TEST(UnitTest_Cycle, convert_operator) {
	EXPECT_EQ(0, Cycle<unsigned short>(0, 2, 0));
	EXPECT_EQ(1, Cycle<unsigned short>(1, 2, 0));
	EXPECT_EQ(2, Cycle<unsigned short>(2, 2, 0));
}

TEST(UnitTest_Cycle, get) {
	EXPECT_EQ(0, Cycle<unsigned short>(0, 2, 0).get());
	EXPECT_EQ(1, Cycle<unsigned short>(1, 2, 0).get());
	EXPECT_EQ(2, Cycle<unsigned short>(2, 2, 0).get());
}

TEST(UnitTest_Cycle, set) {
	EXPECT_EQ(2, Cycle<unsigned short>(0, 2, 0).set(2));
	EXPECT_EQ(0, Cycle<unsigned short>(1, 2, 0).set(0));
	EXPECT_EQ(1, Cycle<unsigned short>(2, 2, 0).set(1));
}

TEST(UnitTest_Cycle, set_bounds) {
	EXPECT_THROW(Cycle<unsigned short>(1, 2, 1).set(0), std::logic_error);
	EXPECT_THROW(Cycle<unsigned short>(1, 2, 1).set(3), std::logic_error);
}

TEST(UnitTest_Cycle, forward) {
	EXPECT_EQ(1, Cycle<unsigned short>(0, 2, 0).forward());
	EXPECT_EQ(2, Cycle<unsigned short>(0, 2, 0).forward().forward());
	EXPECT_EQ(0, Cycle<unsigned short>(0, 2, 0).forward().forward().forward());
}

TEST(UnitTest_Cycle, backward) {
	EXPECT_EQ(2, Cycle<unsigned short>(0, 2, 0).backward());
	EXPECT_EQ(1, Cycle<unsigned short>(0, 2, 0).backward().backward());
	EXPECT_EQ(0, Cycle<unsigned short>(0, 2, 0).backward().backward().backward());
}

