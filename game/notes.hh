#pragma once

#include <map>
#include <string>
#include <vector>

#include "color.hh"
#include "musicalscale.hh"

/// stores duration of a note
struct Duration {
	float begin; ///< beginning timestamp in seconds
	float end;   ///< ending timestamp in seconds
	Duration();
	/// create a new Duration object and initialize begin and end
	Duration(float b, float e): begin(b), end(e) {}
	/// compares begin timestamps of two Duration structs
	static bool ltBegin(Duration const& a, Duration const& b) { return a.begin < b.begin; }
	/// compares end timestamps of two Duration structs
	static bool ltEnd(Duration const& a, Duration const& b) { return a.end < b.end; }
};

typedef std::vector<Duration> Durations;
typedef std::map<int, Durations> NoteMap;

/// Sort by instrument track name
struct CompInstrumentTrack {
	bool operator()(std::string const& l, std::string const& r) const {
		// TODO: Sort other guitar tracks (coop / rhythm) properly
		return l > r;
	}
};

struct InstrumentTrack {
	// TODO: name should not be needed here (contained into the map)
	InstrumentTrack(std::string n): name(n) {}
	std::string name;
	NoteMap nm;
};

// keep these ones
typedef std::map<std::string, InstrumentTrack, CompInstrumentTrack> InstrumentTracks;
typedef std::map<std::string, InstrumentTrack const*, CompInstrumentTrack> InstrumentTracksConstPtr; // this one really needed ? can't we save only the map key for comparison ?

static inline bool isTrackInside(InstrumentTracks const& track_map, std::string const& name) {
	return track_map.find(name) != track_map.end();
}

// TODO: Make Note use Duration

/// note read from songfile
struct Note {
	Note();
	float begin; ///< begin time
	float end; ///< end time
	float phase; /// Position within a measure, [0, 1)
	// FIXME: Remove gameplay variables from here (Note should be immutable).
	/// power of note (how well it is being hit right now)
	mutable float power;
	/// which players sung well
	mutable std::vector<Color> stars;
	/// note type
	enum class Type { FREESTYLE = 'F', NORMAL = ':', GOLDEN = '*', GOLDEN2 = 'G', SLIDE = '+', SLEEP = '-', RAP = 'R',
	  TAP = '1', HOLDBEGIN = '2', HOLDEND = '3', ROLL = '4', MINE = 'M', LIFT = 'L'} type;
	int note; ///< MIDI pitch of the note (at the end for slide notes)
	int notePrev; ///< MIDI pitch of the previous note (should be same as note for everything but SLIDE)
	/// lyrics syllable for that note
	std::string syllable;
	/// Difference of n from note
	float diff(float n) const { return diff(note, n); }
	/// Difference of n from note, so that note + diff(note, n) is n (mod 12)
	static float diff(float note, float n);
	/// Maximum score
	float maxScore() const;
	/// The length of the time period [a,b] that falls within the note in seconds
	float clampDuration(float b, float e) const;
	/// Score when singing over time period (a, b), which needs not to be entirely within the note
	float score(float freq, float b, float e) const;
	/// How precisely the note is hit (always 1.0 for freestyle, 0..1 for others)
	float powerFactor(float note) const;
	/// Compares begin of two notes
	static bool ltBegin(Note const& a, Note const& b) {
		if (a.begin == b.begin) {
			if (a.type == b.type) return false;
			if (a.type == Note::Type::SLEEP) return true;
			if (b.type == Note::Type::SLEEP) return false;
		}
		return a.begin < b.begin; 
	}
	/// Compares end of two notes
	static bool ltEnd(Note const& a, Note const& b) { return a.end < b.end; }
	/// Compare equality of two notes, used for deleting duplicates when programatically creating the duet track.
	static bool equal(Note const& a, Note const& b) { 
		if (a.type == Note::Type::SLEEP) return (a.type == b.type);
		return (a.begin == b.begin && a.end == b.end && a.note == b.note && a.type == b.type);
	}
	/// Check if two notes overlap
	static bool overlapping(Note const& a, Note const& b) { return (a.end > b.begin && a.type != Note::Type::SLEEP && b.type != Note::Type::SLEEP); }
  private:
	  float scoreMultiplier() const;
};


float thresholdForFullScore(); /// Threshold to award perfect score for a note
float thresholdForNonzeroScore(); /// Threshold to award nonzero score for a note


typedef std::vector<Note> Notes;

class VocalTrack {
public:
	VocalTrack(std::string name);
	void reload();
	std::string name;
	Notes notes;
	int noteMin, noteMax; ///< lowest and highest note
	float beginTime, endTime; ///< the period where there are notes
	float m_scoreFactor; ///< normalization factor for the scoring system
	MusicalScale scale; ///< scale in which song is sung
};

typedef std::map<std::string, VocalTrack> VocalTracks;

struct DanceTrack {
	DanceTrack(std::string& description, Notes& notes);
	//track description
	std::string description;
	//container for the actual note data
	Notes notes;
};

enum class GameDifficulty {
	NORMAL,
	HARD,
	PERFECT
};

enum class DanceDifficulty : int {
	BEGINNER,
	EASY,
	MEDIUM,
	HARD,
	CHALLENGE,
	COUNT
};

typedef std::map<DanceDifficulty, DanceTrack> DanceDifficultyMap;
typedef std::map<std::string, DanceDifficultyMap> DanceTracks;
