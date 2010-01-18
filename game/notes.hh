#pragma once

#include <map>
#include <string>
#include <vector>

/// musical scale, defaults to C major
class MusicalScale {
  private:
	double m_baseFreq;
	static const int m_baseId = 33;

  public:
	/// constructor
	MusicalScale(double baseFreq = 440.0): m_baseFreq(baseFreq) {}
	/// get name of note
	std::string getNoteStr(double freq) const;
	/// get note number for id
	unsigned int getNoteNum(int id) const;
	/// true if sharp note
	bool isSharp(int id) const;
	/// get frequence for note id
	double getNoteFreq(int id) const;
	/// get note id for frequence
	int getNoteId(double freq) const;
	/// get note for frequence
	double getNote(double freq) const;
	/// get note offset for frequence
	double getNoteOffset(double freq) const;
};

/// stores duration of a note
struct Duration {
	double begin, ///< beginning timestamp in seconds
	       end;   ///< ending timestamp in seconds
	Duration();
	/// create a new Duration object and initialize begin and end
	Duration(double b, double e): begin(b), end(e) {}
	/// compares begin timestamps of two Duration structs
	static bool ltBegin(Duration const& a, Duration const& b) { return a.begin < b.begin; }
	/// compares end timestamps of two Duration structs
	static bool ltEnd(Duration const& a, Duration const& b) { return a.end < b.end; }
};

typedef std::vector<Duration> Durations;
typedef std::map<int, Durations> NoteMap;

struct Track {
	// TODO: name should not be needed here (contained into the map)
	Track(std::string n): name(n) {}
	std::string name;
	NoteMap nm;
};

// keep these ones
typedef std::map<std::string,Track> TrackMap;
typedef std::map<std::string,Track const*> TrackMapConstPtr; // this one really needed ? can't we save only the map key for comparison ?

namespace {
	bool isTrackInside(TrackMap const& track_map, std::string const& name) {
		return track_map.find(name) != track_map.end();
	}
}

// TODO: Make Note use Duration

/// note read from songfile
struct Note {
	Note();
	double begin, ///< begin time
	       end; ///< end time
	double phase; /// Position within a measure, [0, 1)
	/// power of note (how well it is being hit right now)
	mutable double power;
	/// how well the note was sung [0,1] (used for drawing a star)
	mutable double accuracy;
	/// note type
	enum Type { FREESTYLE = 'F', NORMAL = ':', GOLDEN = '*', SLIDE = '+', SLEEP = '-',
	  TAP = '1', HOLDBEGIN = '2', HOLDEND = '3', ROLL = '4', MINE = 'M', LIFT = 'L'} type;
	int note; ///< MIDI pitch of the note (at the end for slide notes)
	int notePrev; ///< MIDI pitch of the previous note (should be same as note for everything but SLIDE)
	/// lyrics syllable for that note
	std::string syllable;
	/// Difference of n from note
	double diff(double n) const { return diff(note, n); }
	/// Difference of n from note, so that note + diff(note, n) is n (mod 12)
	static double diff(double note, double n);
	/// maximum score
	double maxScore() const;
	/// score when singing over time period (a, b), which needs not to be entirely within the note
	double score(double freq, double b, double e) const;
	/// How precisely the note is hit (always 1.0 for freestyle, 0..1 for others)
	double powerFactor(double note) const;
	/// compares begin of two notes
	static bool ltBegin(Note const& a, Note const& b) { return a.begin < b.begin; }
	/// compares end of two notes
	static bool ltEnd(Note const& a, Note const& b) { return a.end < b.end; }
  private:
	double scoreMultiplier() const;
};

typedef std::vector<Note> Notes;


struct DanceTrack {
	DanceTrack(std::string& description, Notes& notes);
	//track description
	std::string description;
	//container for the actual note data
	Notes notes;	
};

enum DanceDifficulty {
	BEGINNER,
	EASY,
	MEDIUM,
	HARD,
	CHALLENGE,
	DIFFICULTYCOUNT
};

typedef std::map<DanceDifficulty, DanceTrack> DanceDifficultyMap;
typedef std::map<std::string, DanceDifficultyMap> DanceTracks;
