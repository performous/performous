#include <gtest/gtest.h>

#include "game/notegraphscalerfactory.hh"

#include "game/notes.hh"
#include "game/dynamicnotegraphscaler.hh"
#include "game/fixednotegraphscaler.hh"

namespace {
	ConfigItem makeConfigItem(int i) { return ConfigItem{static_cast<unsigned short>(i)};}

	TEST(UnitTest_NoteGraphScalerFactory, dynamic) {
		const auto vocal = VocalTrack("Songname");
		auto config = Config{{"game/notegraphscalingmode", makeConfigItem(0)}};
		const auto scaler = NoteGraphScalerFactory(config).create(vocal);

		ASSERT_NE(nullptr, scaler);
		EXPECT_NE(nullptr, std::dynamic_pointer_cast<DynamicNoteGraphScaler>(scaler));
	}

	TEST(UnitTest_NoteGraphScalerFactory, fixed) {
		const auto vocal = VocalTrack("Songname");
		auto config = Config{{"game/notegraphscalingmode", makeConfigItem(1)}};
		const auto scaler = NoteGraphScalerFactory(config).create(vocal);

		ASSERT_NE(nullptr, scaler);
		EXPECT_NE(nullptr, std::dynamic_pointer_cast<FixedNoteGraphScaler>(scaler));
	}

	TEST(UnitTest_NoteGraphScalerFactory, auto_1) {
		const auto vocal = VocalTrack("Songname");
		auto config = Config{{"game/notegraphscalingmode", makeConfigItem(2)}};
		const auto scaler = NoteGraphScalerFactory(config).create(vocal);

		ASSERT_NE(nullptr, scaler);
		EXPECT_NE(nullptr, std::dynamic_pointer_cast<FixedNoteGraphScaler>(scaler));
	}

	TEST(UnitTest_NoteGraphScalerFactory, auto_2) {
		const auto vocal = VocalTrack("Songname");
		auto config = Config{{"game/notegraphscalingmode", makeConfigItem(3)}};
		const auto scaler = NoteGraphScalerFactory(config).create(vocal);

		ASSERT_NE(nullptr, scaler);
		EXPECT_NE(nullptr, std::dynamic_pointer_cast<FixedNoteGraphScaler>(scaler));
	}

	TEST(UnitTest_NoteGraphScalerFactory, auto_3) {
		const auto vocal = VocalTrack("Songname");
		auto config = Config{{"game/notegraphscalingmode", makeConfigItem(4)}};
		const auto scaler = NoteGraphScalerFactory(config).create(vocal);

		ASSERT_NE(nullptr, scaler);
		EXPECT_NE(nullptr, std::dynamic_pointer_cast<FixedNoteGraphScaler>(scaler));
	}

	TEST(UnitTest_NoteGraphScalerFactory, auto_4) {
		const auto vocal = VocalTrack("Songname");
		auto config = Config{{"game/notegraphscalingmode", makeConfigItem(5)}};
		const auto scaler = NoteGraphScalerFactory(config).create(vocal);

		ASSERT_NE(nullptr, scaler);
		EXPECT_NE(nullptr, std::dynamic_pointer_cast<FixedNoteGraphScaler>(scaler));
	}

}

