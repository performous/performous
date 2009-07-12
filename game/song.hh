#pragma once

#include "notes.hh"
#include <boost/noncopyable.hpp>
#include <stdexcept>

/// parsing of songfile failed
struct SongParserException: public std::runtime_error {
	/// constructor
	SongParserException(std::string const& msg, unsigned int linenum, bool sil = false): runtime_error(msg), m_linenum(linenum), m_silent(sil) {}
	unsigned int line() const { return m_linenum; } ///< line in which the error occured
	bool silent() const { return m_silent; } ///< if the error should not be printed to user (file skipped)
  private:
	unsigned int m_linenum;
	bool m_silent;
};

class SongParser;

/// class to load and parse songfiles
class Song: boost::noncopyable {
	friend class SongParser;
  public:
	/// constructor
	Song(std::string const& path_, std::string const& filename_): path(path_), filename(filename_) { reload(false); }
	/// reload song
	void reload(bool errorIgnore = true);
	/// parse field
	bool parseField(std::string const& line);
	/** Get formatted song label. **/
	std::string str() const { return title + "  by  " + artist; }
	/** Get full song information (used by the search function). **/
	std::string strFull() const { return title + "\n" + artist + "\n" + genre + "\n" + edition + "\n" + path; }
	/// status of song
	enum Status { NORMAL, INSTRUMENTAL_BREAK, FINISHED };
	/** Get the song status at a given timestamp **/
	Status status(double time) const;
	int randomIdx; ///< sorting index used for random order
	Notes notes; ///< notes for song
	int noteMin, ///< lowest note
	    noteMax; ///< highest note
	std::string path; ///< path of songfile
	std::string filename; ///< name of songfile
	std::vector<std::string> category; ///< category of song
	std::string genre; ///< genre
	std::string edition; ///< license
	std::string title; ///< songtitle
	std::string artist; ///< artist
	std::string text; ///< songtext
	std::string creator; ///< creator
	std::vector<std::string> music; ///< music files (background, guitar, rhythm/bass, drums, vocals)
	std::string cover; ///< cd cover
	std::string background; ///< background image
	std::string video; ///< video
	/// Variables used for comparisons (sorting)
	std::string collateByTitle;
	/// Variables used for comparisons (sorting)
	std::string collateByArtist;
	/** Rebuild collate variables from other strings **/
	void collateUpdate();
	/** Convert a string to its collate form **/
	static std::string collate(std::string const& str);
	double videoGap; ///< gap with video
	double start; ///< start of song
	MusicalScale scale; ///< scale in which song is sung
	std::vector<double> timePitchGraph; ///< time of pitch graph
	std::vector<double> pitchPitchGraph; ///< pitch of pitch graph
	std::vector<double> volumePitchGraph; ///< volume of pitch graph
	std::vector<bool> drawPitchGraph; ///< if pitch graph should be drawn
	double m_scoreFactor; ///< Normalization factor for the scoring system
};

static inline bool operator<(Song const& l, Song const& r) { return l.collateByArtist < r.collateByArtist; }

