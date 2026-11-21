#include "gamereplay.hh"

#include "game/animvalue.hh"
#include "game/guitarengine.hh"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <deque>
#include <stdexcept>

namespace {
	Time g_now;
	Time replayClock() { return g_now; }

	struct ClockOverride {
		ClockOverride() { g_now = Time(); animclock::now = &replayClock; }
		~ClockOverride() { animclock::now = &animclock::realNow; }
	};

	void setWall(double wall) { g_now = Time() + std::chrono::duration_cast<Clock::duration>(Seconds(wall)); }

	using Difficulty = GuitarEngine::Difficulty;

	GuitarEngine::Settings settingsOf(nlohmann::json const& s) {
		GuitarEngine::Settings out;
		out.missPenalty = s.at("game/instrument_miss_penalty").get<bool>();
		out.missVolumePercent[to_underlying(Difficulty::KIDS)] = s.at("audio/miss_volume_kids").get<unsigned>();
		out.missVolumePercent[to_underlying(Difficulty::SUPAEASY)] = s.at("audio/miss_volume_easy").get<unsigned>();
		out.missVolumePercent[to_underlying(Difficulty::EASY)] = s.at("audio/miss_volume_medium").get<unsigned>();
		out.missVolumePercent[to_underlying(Difficulty::MEDIUM)] = s.at("audio/miss_volume_hard").get<unsigned>();
		out.missVolumePercent[to_underlying(Difficulty::AMAZING)] = s.at("audio/miss_volume_expert").get<unsigned>();
		return out;
	}

	InstrumentTrack trackOf(nlohmann::json const& chart) {
		InstrumentTrack track(chart.at("track").get<std::string>());
		for (auto const& [pitch, notes]: chart.at("notes").items()) {
			Durations& d = track.nm[static_cast<unsigned>(std::stoul(pitch))];
			for (auto const& n: notes) d.push_back(Duration(n[0].get<double>(), n[1].get<double>()));
		}
		return track;
	}

	struct State {
		double score = 0.0, starmeter = 0.0;
		int streak = 0, longestStreak = 0;
	};
}

ReplayResult replayPlayer(nlohmann::json const& report, nlohmann::json const& player, double scoreTolerance) {
	ClockOverride clock;  // Before the engine: its AnimValues read the clock when constructed
	GuitarEngine::Listener listener;
	bool const drums = player.at("instrument").get<std::string>() == "drums";
	GuitarEngine engine(listener, drums, player.at("proMode").get<bool>(), report.at("song").at("hasBRE").get<bool>());
	engine.settings = settingsOf(player.at("settings"));
	std::deque<InstrumentTrack> tracks;  // Stable addresses: the engine points into them
	for (auto const& c: player.at("charts")) tracks.push_back(trackOf(c));
	double nextNoise = 0.0;
	engine.whammyNoise = [&nextNoise] { return nextNoise; };

	ReplayResult result;
	State expected;   // Last state recorded
	bool frameOpen = false;
	double frameTime = 0.0;
	auto compare = [&](State const& want, char const* where, std::size_t index) {
		++result.checks;
		result.maxScoreDiff = std::max(result.maxScoreDiff, std::abs(engine.score() - want.score) / std::max(1.0, std::abs(want.score)));
		result.maxStarmeterDiff = std::max(result.maxStarmeterDiff, std::abs(engine.starmeter() - want.starmeter) / std::max(1.0, std::abs(want.starmeter)));
		auto close = [](double a, double b, double tol) { return std::abs(a - b) <= tol * std::max(1.0, std::abs(b)); };
		bool ok = close(engine.score(), want.score, scoreTolerance) && close(engine.starmeter(), want.starmeter, 1e-12)
		  && engine.streak() == want.streak && engine.longestStreak() == want.longestStreak;
		if (ok) return;
		if (result.mismatches++ < 5) {
			result.firstMismatches.push_back(fmt::format(
			  "entry {} ({}) at song time {:.3f}: score {} vs recorded {}, streak {}/{} vs {}/{}, star meter {} vs {}",
			  index, where, frameTime, engine.score(), want.score, engine.streak(), engine.longestStreak(),
			  want.streak, want.longestStreak, engine.starmeter(), want.starmeter));
		}
	};
	// A frame ends with the engine's update. Its state is recorded right after when it changed,
	// otherwise it must still be the last one recorded (checked before drawing changes anything).
	auto endFrame = [&](std::size_t index, bool stateFollows) {
		if (!frameOpen) return;
		engine.update(frameTime);
		frameOpen = false;
		if (!stateFollows) compare(expected, "frame", index);
	};

	auto const& entries = player.at("events");
	for (std::size_t i = 0; i < entries.size(); ++i) {
		auto const& e = entries[i];
		std::string const type = e[0].get<std::string>();
		if (type == "f") {
			endFrame(i, false);
			frameTime = e[1].get<double>();
			setWall(e[2].get<double>());
			engine.beginFrame(frameTime);
			frameOpen = true;
			++result.frames;
		} else if (type == "p") {
			engine.setPressed(e[1].get<unsigned>(), e[2].get<bool>());
		} else if (type == "i") {
			input::Event ev;
			ev.button = input::ButtonId(e[1].get<unsigned>());
			ev.value = e[2].get<double>();
			// Recorded whammy = (1 + value + 2 * noise) / 4
			nextNoise = (4.0 * e[3].get<double>() - 1.0 - ev.value) / 2.0;
			engine.input(frameTime, ev);
		} else if (type == "s") {
			endFrame(i, true);
			expected = {e[1].get<double>(), e[4].get<double>(), e[2].get<int>(), e[3].get<int>()};
			compare(expected, "state", i);
		} else if (type == "d") {
			endFrame(i, false);
			setWall(e[2].get<double>());
			engine.correctness().get();  // Drawing reads it (and the screen right after), which moves the animation
			engine.updateLookahead(e[1].get<double>());
		} else if (type == "c") {
			auto const index = e[1].get<std::size_t>();
			auto const& chart = player.at("charts").at(index);
			if (!engine.setDifficulty(tracks.at(index), Difficulty(chart.at("level").get<int>())))
				throw std::runtime_error("Recorded difficulty not available in the recorded chart");
		} else if (type == "j") {
			engine.resetScore();
		} else {
			throw std::runtime_error("Unknown entry type " + type);
		}
	}
	endFrame(entries.size(), false);
	result.finalScore = engine.score();
	result.longestStreak = engine.longestStreak();
	return result;
}
