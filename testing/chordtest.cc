#include "common.hh"

#include "game/guitarchords/chord.hh"

TEST(UnitTest_Chord, ctor) {
	auto const chord = Chord("name", {});

	EXPECT_EQ("name", chord.getName());
	EXPECT_THAT(chord.getFrets(), ElementsAre());
	//EXPECT_EQ(0, chord.countAlternatives());
}

TEST(UnitTest_Chord, getFrets) {
	auto const chord = Chord("name", {0, 1, 2, 3, 4, 5});

	EXPECT_THAT(chord.getFrets(), ElementsAre(0, 1, 2, 3, 4, 5));
//	EXPECT_EQ(0, chord.countAlternatives());
}
/*
TEST(UnitTest_Chord, getAlternative_none) {
	auto const chord = Chord("name", {440.f, 500.f});

	EXPECT_EQ(0, chord.countAlternatives());
	EXPECT_THROW(chord.getAlternative(), std::exception);
}

TEST(UnitTest_Chord, addAlternative) {
	auto const chord = Chord("name", {440.f, 500.f}).addAlternate({220.f, 500.f});

	EXPECT_THAT(chord.getFrequencies(), ElementsAre(440.f, 500.f));
	EXPECT_EQ(1, chord.countAlternatives());
	EXPECT_THAT(chord.getAlternative(), ElementsAre(220.f, 500.f));
}

TEST(UnitTest_Chord, addAlternatives) {
	auto const chord = Chord("name", {440.f, 500.f})
		.addAlternate({220.f, 500.f})
		.addAlternate({220.f, 250.f});

	EXPECT_THAT(chord.getFrequencies(), ElementsAre(440.f, 500.f));
	EXPECT_EQ(2, chord.countAlternatives());
	EXPECT_THAT(chord.getAlternative(), ElementsAre(220.f, 500.f));
	EXPECT_THAT(chord.getAlternative(1), ElementsAre(220.f, 250.f));
}
*/
