#pragma once

#include "chrono.hh"
#include "fs.hh"
#include "notes.hh"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

/// Records what a guitar/drums player did during a song so that it can be saved as a bug report and
/// replayed through the gameplay rules.
///
/// The recording is a flat list of the calls made to the gameplay rules, in order:
/// - frame: start of a GuitarGraph::engine() call, with the song time it uses
/// - pressed: a guitar fret held/released (num = fret + 1)
/// - input: an event that reached the gameplay rules
/// - draw: the song time of a drawNotes() call (it changes game state)
///
/// Frames and draws also store the wall-clock time since the recording started: animated values
/// (correctness, GodMode) advance with it, and they are read while drawing too.
/// - state: score, streak and star meter at the end of a frame, when they changed
/// - chart: the track and difficulty in use from now on
/// - rejoin: the player rejoined (scores reset)
///
/// It contains no audio, no lyrics, no names and no paths.
class GameRecorder {
  public:
	GameRecorder();
	void frame(double songTime);
	void pressed(unsigned num, bool value);
	void input(unsigned button, double value, double whammy);
	void draw(double songTime);
	void state(double score, int streak, int longestStreak, double starmeter);
	void chart(std::string const& track, unsigned level);
	void rejoin();

	struct Chart { std::string track; unsigned level; };
	std::vector<Chart> const& charts() const { return m_charts; } ///< Every track/difficulty used, in order
	nlohmann::json entries() const;

  private:
	struct Entry {
		char type;
		double time;  ///< Song time (frame, draw), button value (input, pressed)
		double wall;  ///< Seconds since the recording started (frame, draw), whammy value after the event (input)
		unsigned arg; ///< Button (input), fret + 1 (pressed), chart index (chart)
	};
	std::vector<Entry> m_entries;
	std::vector<Chart> m_charts;
	Time m_start;
	double m_score = 0.0, m_starmeter = 0.0;
	int m_streak = 0, m_longestStreak = 0;
};

/// The notes of a track that the guitar rules read for one difficulty (frets, solos, drum fills, toms)
nlohmann::json chartToJson(InstrumentTrack const& track, unsigned basepitch);

/// Identify a song by the content of its notes file, without naming it
std::string songFingerprint(fs::path const& notesFile);

/// Write a report into the user data folder; returns the file written
fs::path saveGameReport(nlohmann::json const& report);
