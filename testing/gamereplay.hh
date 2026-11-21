#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

/// Result of replaying one player of a game report through GuitarEngine
struct ReplayResult {
	unsigned frames = 0;      ///< Frames replayed
	unsigned checks = 0;      ///< States compared
	unsigned mismatches = 0;  ///< States that differ from the recording
	std::vector<std::string> firstMismatches;  ///< Details of the first few
	double maxScoreDiff = 0.0;      ///< Largest relative score difference seen
	double maxStarmeterDiff = 0.0;  ///< Largest relative star meter difference seen
	double finalScore = 0.0;
	int longestStreak = 0;
};

/// Replay the recorded calls of a game report (see GameRecorder) through GuitarEngine, with the
/// recorded wall-clock time and whammy values, and compare score, streak and star meter after every
/// frame. Streaks and star meter must be equal, the score may differ by the relative `scoreTolerance`:
/// held notes score with an animated value (correctness) that is read a little after the recorded time.
ReplayResult replayPlayer(nlohmann::json const& report, nlohmann::json const& player, double scoreTolerance);
