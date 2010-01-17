#pragma once

#include "notes.hh"
#include <boost/noncopyable.hpp>

#include <stdexcept>
#include <string>

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
	Notes notes; ///< notes for song (only used for singing)
	TrackMap track_map; ///< guitar etc. notes for this song
	DanceTracks danceTracks; ///< dance tracks
	typedef std::vector<double> Beats;
	Beats beats;
	bool hasDance() const { return !danceTracks.empty(); }
	bool hasDrums() const { return track_map.find("drums") != track_map.end(); }
	bool hasGuitars() const { return track_map.size() - hasDrums(); }
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
	std::string language; ///< language
	std::map<std::string,std::string> music; ///< music files (background, guitar, rhythm/bass, drums, vocals)
	std::string cover; ///< cd cover
	std::string background; ///< background image
	std::string video; ///< video
	/// Variables used for comparisons (sorting)
	std::string collateByTitle;
	std::string collateByTitleOnly;
	/// Variables used for comparisons (sorting)
	std::string collateByArtist;
	std::string collateByArtistOnly;
	/** Rebuild collate variables from other strings **/
	void collateUpdate();
	/** Convert a string to its collate form **/
	static std::string collate(std::string const& str);
	double videoGap; ///< gap with video
	double start; ///< start of song
	double preview_start; ///< starting time for the preview
	MusicalScale scale; ///< scale in which song is sung
	std::vector<double> timePitchGraph; ///< time of pitch graph
	std::vector<double> pitchPitchGraph; ///< pitch of pitch graph
	std::vector<double> volumePitchGraph; ///< volume of pitch graph
	std::vector<bool> drawPitchGraph; ///< if pitch graph should be drawn
	double beginTime, endTime; ///< the period where there are notes
	double m_scoreFactor; ///< Normalization factor for the scoring system
	typedef std::vector<std::pair<double,double> > Stops;
	Stops stops;
};

static inline bool operator<(Song const& l, Song const& r) { return l.collateByArtist < r.collateByArtist; }

