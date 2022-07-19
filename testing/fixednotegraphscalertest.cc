#include "game/fixednotegraphscaler.hh"

#include "game/notes.hh"
#include "game/dynamicnotegraphscaler.hh"
#include "game/fixednotegraphscaler.hh"

#include "common.hh"

namespace {
	Note make(float note, std::string const& text, double begin = 0., double end = 1., Note::Type type = Note::Type::NORMAL) {
		auto result = Note();

		result.note = note;
		result.syllable = text;
		result.begin = begin;
		result.end = end;
		result.type = type;

		return result;
	}

	TEST(UnitTest_FixedNoteGraphScaler, calculate_1_time_0) {
		auto vocal = VocalTrack("Songname");

		vocal.noteMin = 0.f;
		vocal.noteMax = 0.f;
		vocal.notes.emplace_back(make(0.f, "A"));

		auto scaler = FixedNoteGraphScaler();

		scaler.initialize(vocal);

		const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 0);

		EXPECT_EQ(0.f, dimension.min1);
		EXPECT_EQ(0.f, dimension.max1);
		EXPECT_EQ(0.f, dimension.min2);
		EXPECT_EQ(0.f, dimension.max2);
	}

	TEST(UnitTest_FixedNoteGraphScaler, calculate_1_time_1) {
		auto vocal = VocalTrack("Songname");

		vocal.noteMin = 0.f;
		vocal.noteMax = 0.f;
		vocal.notes.emplace_back(make(0.f, "A"));

		auto scaler = FixedNoteGraphScaler();

		scaler.initialize(vocal);

		const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 1);

		EXPECT_EQ(0.f, dimension.min1);
		EXPECT_EQ(0.f, dimension.max1);
		EXPECT_EQ(0.f, dimension.min2);
		EXPECT_EQ(0.f, dimension.max2);
	}

	TEST(UnitTest_FixedNoteGraphScaler, calculate_3_time_0) {
		auto vocal = VocalTrack("Songname");

		vocal.noteMin = 0.f;
		vocal.noteMax = 0.f;
		vocal.notes.emplace_back(make(0.f, "A"));
		vocal.notes.emplace_back(make(1.f, "A", 1., 2.));
		vocal.notes.emplace_back(make(3.f, "A", 2., 3.));

		auto scaler = FixedNoteGraphScaler();

		scaler.initialize(vocal);

		const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 0);

		EXPECT_EQ(0.f, dimension.min1);
		EXPECT_EQ(3.f, dimension.max1);
		EXPECT_EQ(0.f, dimension.min2);
		EXPECT_EQ(3.f, dimension.max2);
	}

	TEST(UnitTest_FixedNoteGraphScaler, calculate_3_time_1) {
		auto vocal = VocalTrack("Songname");

		vocal.noteMin = 0.f;
		vocal.noteMax = 0.f;
		vocal.notes.emplace_back(make(0.f, "A"));
		vocal.notes.emplace_back(make(1.f, "A", 1., 2.));
		vocal.notes.emplace_back(make(3.f, "A", 2., 3.));

		auto scaler = FixedNoteGraphScaler();

		scaler.initialize(vocal);

		const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 1);

		EXPECT_EQ(0.f, dimension.min1);
		EXPECT_EQ(3.f, dimension.max1);
		EXPECT_EQ(0.f, dimension.min2);
		EXPECT_EQ(3.f, dimension.max2);
	}

	TEST(UnitTest_FixedNoteGraphScaler, calculate_vocal_min) {
		auto vocal = VocalTrack("Songname");

		vocal.noteMin = 0.f;
		vocal.noteMax = 2.f;
		vocal.notes.emplace_back(make(1.f, "A"));
		vocal.notes.emplace_back(make(2.f, "A", 1., 2.));
		vocal.notes.emplace_back(make(3.f, "A", 2., 3.));

		auto scaler = FixedNoteGraphScaler();

		scaler.initialize(vocal);

		const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 1);

		EXPECT_EQ(1.f, dimension.min1);
		EXPECT_EQ(3.f, dimension.max1);
		EXPECT_EQ(1.f, dimension.min2);
		EXPECT_EQ(3.f, dimension.max2);
	}

	TEST(UnitTest_FixedNoteGraphScaler, calculate_vocal_max) {
		auto vocal = VocalTrack("Songname");

		vocal.noteMin = 2.f;
		vocal.noteMax = 5.f;
		vocal.notes.emplace_back(make(0.f, "A"));
		vocal.notes.emplace_back(make(2.f, "A", 1., 2.));
		vocal.notes.emplace_back(make(3.f, "A", 2., 3.));

		auto scaler = FixedNoteGraphScaler();

		scaler.initialize(vocal);

		const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 1);

		EXPECT_EQ(0.f, dimension.min1);
		EXPECT_EQ(3.f, dimension.max1);
		EXPECT_EQ(0.f, dimension.min2);
		EXPECT_EQ(3.f, dimension.max2);
	}

	TEST(UnitTest_FixedNoteGraphScaler, calculate_ignore_SLEEP) {
		auto vocal = VocalTrack("Songname");

		vocal.noteMin = 0.f;
		vocal.noteMax = 3.f;
		vocal.notes.emplace_back(make(0.f, "A", 0., 1.));
		vocal.notes.emplace_back(make(3.f, "A", 1., 2., Note::Type::SLEEP));
		vocal.notes.emplace_back(make(1.f, "A", 2., 3.));

		auto scaler = FixedNoteGraphScaler();

		scaler.initialize(vocal);

		const auto dimension = scaler.calculate(vocal, vocal.notes.begin(), 0);

		EXPECT_EQ(0.f, dimension.min1);
		EXPECT_EQ(1.f, dimension.max1);
		EXPECT_EQ(0.f, dimension.min2);
		EXPECT_EQ(1.f, dimension.max2);
	}
}


