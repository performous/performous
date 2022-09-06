#include "common.hh"

#include "game/guitarchords/string.hh"
#include "game/guitarchords/notes.hh"

TEST(UnitTest_String, hasInverseFrequency_E0) {
	EXPECT_THAT(String(E0).hasInverseFrequency(0), IsFalse());
}

TEST(UnitTest_String, hasInverseFrequency_E1) {
	EXPECT_THAT(String(E0).hasInverseFrequency(1), IsFalse());
}

TEST(UnitTest_String, hasInverseFrequency_E2) {
	EXPECT_THAT(String(E0).hasInverseFrequency(2), IsTrue());
}

TEST(UnitTest_String, getFrequency_E0) {
	EXPECT_THAT(String(E0).getFrequency(0), FloatNear(E0, 0.01));
}

TEST(UnitTest_String, getFrequency_E1) {
	EXPECT_THAT(String(E0).getFrequency(1), FloatNear(E1, 0.1));
}

TEST(UnitTest_String, getFrequency_E2) {
	EXPECT_THAT(String(E0).getFrequency(2), FloatNear(E2, 0.1));
}

TEST(UnitTest_String, getFrequency_E3) {
	EXPECT_THAT(String(E0).getFrequency(3), FloatNear(E3, 0.1));
}

TEST(UnitTest_String, getFrequency_E0_inverse) {
	EXPECT_THAT(String(E0).getFrequency(0, true), FloatNear(0, 0.01));
}

TEST(UnitTest_String, getFrequency_E1_inverse) {
	EXPECT_THAT(String(E0).getFrequency(1, true), FloatNear(0, 0.01));
}

TEST(UnitTest_String, getFrequency_E2_inverse) {
	EXPECT_THAT(String(E0).getFrequency(2, true), FloatNear(1468.31, 0.01));
}

TEST(UnitTest_String, getFrequency_E3_inverse) {
	EXPECT_THAT(String(E0).getFrequency(3, true), FloatNear(755.35, 0.01));
}

