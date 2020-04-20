#pragma once
#include "color.hh"
#include "fs.hh"
#include "pitch.hh"
#include "notes.hh"
#include "animvalue.hh"

#include <string>
#include <vector>
#include <utility>

#include <boost/optional.hpp>

class Song;

struct Player {
	/// currently played vocal track
	VocalTrack& m_vocal;
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
	/// score for current note
	double m_noteScore;
	/// score for current line
	double m_lineScore;
	/// maximum score for the previous line
	double m_maxLineScore;
	/// score for the previous line (normalized [0,1])
	double m_prevLineScore;
	/// fader for text feedback display
	AnimValue m_feedbackFader;
	/// activity timer
	unsigned m_activitytimer;
	/// score iterator
	Notes::const_iterator m_scoreIt;
	/// constructor
	Player(VocalTrack& vocal, Analyzer& analyzer, size_t frames);

	std::string const& getId() const { return m_analyzer.getId(); }

	/// prepares analyzer
	void prepare() { m_analyzer.process(); }
	/// updates player stats
	void update();
	/// calculate how well last lyrics row went
	void calcRowRank();
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
	int id; ///< unique identifier for this PlayerItem, Link to hiscore

	std::string name; ///< name displayed and used for searching the player
	fs::path picture; ///< the filename which was passed from xml (and is written back)
	fs::path path; ///< a full path to a picture shown, generated from picture above
/* Future ideas
	std::string displayedName; /// artist name, short name, nick (can be changed)
	std::map<std::string, int> scores; /// map between a Song and the highest score the Player achieved
*/

	/**For insertion in set.
	 Provides ordering and ensures id is unique.*/
	bool operator< (PlayerItem const& pi) const
	{
		return id < pi.id;
	}

	/**Checks if a player has the same name.
	  Used to find a PlayerItem with the same name.*/
	bool operator== (PlayerItem const& pi) const
	{
		return name == pi.name;
	}
};
