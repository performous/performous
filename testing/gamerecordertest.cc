#include "common.hh"

#include "game/gamerecorder.hh"

#include <filesystem>
#include <fstream>

TEST(GameRecorder, RecordsCallsInOrder) {
	GameRecorder rec;
	rec.chart("Guitar", 4);
	rec.frame(1.5);
	rec.pressed(1, true);
	rec.input(11, 1.0, 0.0);
	rec.state(50.0, 1, 1, 50.0);
	rec.draw(1.52);
	auto const e = rec.entries();
	ASSERT_EQ(e.size(), 6u);
	EXPECT_EQ(e[0], nlohmann::json({"c", 0}));
	EXPECT_EQ(e[1][0], "f");
	EXPECT_DOUBLE_EQ(e[1][1].get<double>(), 1.5);
	EXPECT_EQ(e[2], nlohmann::json({"p", 1, true}));
	EXPECT_EQ(e[3], nlohmann::json({"i", 11, 1.0, 0.0}));
	EXPECT_EQ(e[4], nlohmann::json({"s", 50.0, 1, 1, 50.0}));
	EXPECT_EQ(e[5][0], "d");
	EXPECT_DOUBLE_EQ(e[5][1].get<double>(), 1.52);
	EXPECT_GE(e[5][2].get<double>(), e[1][2].get<double>());  // Wall-clock time never goes back
}

TEST(GameRecorder, StateIsOnlyRecordedWhenItChanges) {
	GameRecorder rec;
	rec.state(0.0, 0, 0, 0.0);  // Same as the initial state
	rec.state(30.0, 1, 1, 30.0);
	rec.state(30.0, 1, 1, 30.0);
	EXPECT_EQ(rec.entries().size(), 1u);
}

TEST(GameRecorder, ChartsAreListedOnce) {
	GameRecorder rec;
	rec.chart("Guitar", 4);
	rec.chart("Bass", 3);
	rec.chart("Guitar", 4);
	ASSERT_EQ(rec.charts().size(), 2u);
	EXPECT_EQ(rec.entries()[2], nlohmann::json({"c", 0}));
}

TEST(GameRecorder, ChartContainsOnlyTheNotesTheRulesRead) {
	InstrumentTrack track("Guitar");
	track.nm[0x60].push_back(Duration(1.0, 1.5));  // Expert green
	track.nm[0x54].push_back(Duration(2.0, 2.0));  // Hard: not this difficulty
	track.nm[103].push_back(Duration(0.0, 10.0));  // Solo
	auto const chart = chartToJson(track, 0x60);
	EXPECT_EQ(chart, nlohmann::json::parse(R"({"96": [[1.0, 1.5]], "103": [[0.0, 10.0]]})"));
}

TEST(GameRecorder, FingerprintIdentifiesContentNotName) {
	auto const dir = std::filesystem::temp_directory_path();
	auto const a = dir / "performous_fingerprint_a.txt", b = dir / "performous_fingerprint_b.txt";
	std::ofstream(a) << "a";
	std::ofstream(b) << "a";
	EXPECT_EQ(songFingerprint(a), "fnv1a64:af63dc4c8601ec8c");  // Reference FNV-1a value of "a"
	EXPECT_EQ(songFingerprint(a), songFingerprint(b));
	EXPECT_EQ(songFingerprint(a).find("performous_fingerprint"), std::string::npos);
	std::filesystem::remove(a);
	std::filesystem::remove(b);
}
