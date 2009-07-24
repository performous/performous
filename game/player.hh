#pragma once
#include "songs.hh"
#include "color.hh"
#include "pitch.hh"
#include "util.hh"

#include <vector>
#include <limits>


/// player class
struct Player {
	/// currently playing song
	Song& m_song;
	/// sound analyzer
	Analyzer& m_analyzer;
	/// player color for bars, waves, scores
	Color m_color;
	/// typedef for pitch
	typedef std::vector<std::pair<double, double> > pitch_t;
	/// player's pitch
	pitch_t m_pitch;
	/// current position in pitch vector (first unused spot)
	size_t m_pos;
	/// score for current song
	double m_score;
	/// activity timer
	unsigned m_activitytimer;
	/// score iterator
	Notes::const_iterator m_scoreIt;
	/// constructor
	Player(Song& song, Analyzer& analyzer, size_t frames): m_song(song), m_analyzer(analyzer), m_pitch(frames, std::make_pair(getNaN(), -getInf())), m_pos(), m_score(), m_activitytimer(), m_scoreIt(m_song.notes.begin()) {}
	/// prepares analyzer
	void prepare() { m_analyzer.process(); }
	/// updates player stats
	void update();
	/// player activity singing
	float activity() const { return m_activitytimer / 300.0; }
	/// get player's score
	int getScore() const {
		return 10000.0 * m_score;
	}
};
