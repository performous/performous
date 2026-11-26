#include "common.hh"
#include "gamereplay.hh"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

// Games recorded with the gameplay code from before GuitarEngine existed (GuitarGraph doing it all),
// replayed through GuitarEngine: every frame must end with the same score, streak and star meter.
// The recordings come from scripted players (testing/replays/README.md).

namespace {
	std::vector<std::filesystem::path> reports() {
		std::vector<std::filesystem::path> out;
		for (auto const& entry: std::filesystem::directory_iterator(REPLAY_DIR))
			if (entry.path().extension() == ".json") out.push_back(entry.path());
		std::sort(out.begin(), out.end());
		return out;
	}

	nlohmann::json load(std::filesystem::path const& file) {
		std::ifstream f(file);
		return nlohmann::json::parse(f);
	}

	// Relative, for the score only. Measured: below 1e-6 (the engine reads correctness a little after
	// the recorded frame time). One note more or less is 50 points and changes the streak.
	const double scoreTolerance = 1e-5;
}

TEST(GameReplay, RecordedGamesExist) {
	EXPECT_GE(reports().size(), 8u);
}

TEST(GameReplay, RecordedGamesPlayTheSame) {
	for (auto const& file: reports()) {
		auto const report = load(file);
		for (auto const& player: report.at("players")) {
			// The file name is in each message: SCOPED_TRACE crashes when its scope ends with MSVC (gtest DLL)
			std::string const name = file.filename().string();
			auto const r = replayPlayer(report, player, scoreTolerance);
			EXPECT_GT(r.frames, 500u) << name;
			std::cout << name << ": " << r.frames << " frames, " << r.checks << " checks, score " << r.finalScore << ", max score diff " << r.maxScoreDiff << ", max star meter diff " << r.maxStarmeterDiff << std::endl;
			EXPECT_EQ(r.mismatches, 0u) << name << ": of " << r.checks << " checks";
			for (auto const& m: r.firstMismatches) ADD_FAILURE() << name << ": " << m;
		}
	}
}

TEST(GameReplay, DetectsADifferentOutcome) {
	auto report = load(reports().at(0));
	auto& player = report.at("players").at(0);
	// Pretend the recording scored one more note (50 points) at some state change
	for (auto& e: player.at("events")) {
		if (e[0] == "s" && e[1].get<double>() > 1000.0) { e[1] = e[1].get<double>() + 50.0; break; }
	}
	auto const r = replayPlayer(report, player, scoreTolerance);
	EXPECT_GE(r.mismatches, 1u);
}
