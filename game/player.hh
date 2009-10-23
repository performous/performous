#pragma once
#include "song.hh"
#include "color.hh"
#include "pitch.hh"
#include "util.hh"

#include <vector>

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
	/// score for current line
	double m_lineScore;
	/// maximum score for the current line
	double m_maxLineScore;
	/// score for the previous line (normalized)
	double m_prevLineScore;
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
	/**Operator for sorting by score.*/
	bool operator < (Player const& other) const
	{
		return other.m_score < m_score;
	}
};

/** Static Information of a player, not
  dependent from current song.

  Used for Players Management.
  */
struct PlayerItem {
	std::string name; /// unique name, link to highscore
	std::string path; /// a path to a picture shown
	std::string picture; /// + the filename for it
/* Future ideas
	std::string displayedName; /// artist name, short name, nick (can be changed)
	std::map<std::string, int> scores; /// map between a Song and the highest score the Player achieved
*/

	bool operator== (PlayerItem const& pi) const
	{
		return name == pi.name;
	}
};
