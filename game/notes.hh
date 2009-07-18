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

struct Duration {
	double begin, end; ///< Beginning and ending timestamps is seconds
	Duration();
	Duration(double b, double e): begin(b), end(e) {}
	static bool ltBegin(Duration const& a, Duration const& b) { return a.begin < b.begin; }
	static bool ltEnd(Duration const& a, Duration const& b) { return a.end < b.end; }
};

typedef std::vector<Duration> Durations;
typedef std::map<int, Durations> NoteMap;
typedef std::map<std::string, NoteMap> Tracks;

// TODO: Make Note use Duration

/// note read from songfile
struct Note {
	Note();
	double begin, ///< begin time
	       end; ///< end time
	/// power of note
	mutable double power;
	/// note type
	enum Type { FREESTYLE = 'F', NORMAL = ':', GOLDEN = '*', SLIDE = '+', SLEEP = '-'} type;
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
	/// score when singing
	double score(double freq, double b, double e) const;
	static bool ltBegin(Note const& a, Note const& b) { return a.begin < b.begin; }
	static bool ltEnd(Note const& a, Note const& b) { return a.end < b.end; }
  private:
	double scoreMultiplier(double error) const;
};

typedef std::vector<Note> Notes;

