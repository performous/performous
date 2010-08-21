#pragma once

#include "notes.hh"
#include "i18n.hh"
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

namespace TrackName {
	const std::string GUITAR = "Guitar";
	const std::string GUITAR_COOP = "Coop guitar";
	const std::string GUITAR_RHYTHM = "Rhythm guitar";
	const std::string BASS = "Bass";
	const std::string DRUMS = "Drums";
	#if 0 // Here is some dummy gettext calls to populate the dictionary
	_("Guitar") _("Coop guitar") _("Rhythm guitar") _("Bass") _("Drums")
	#endif
}

/// class to load and parse songfiles
class Song: boost::noncopyable {
	friend class SongParser;
  public:
	/// constructor
	Song(std::string const& path_, std::string const& filename_): vocals(std::string("VOCALS")), path(path_), filename(filename_) { reload(false); }
	/// reload song
	void reload(bool errorIgnore = true);
	/// parse field
	bool parseField(std::string const& line);
	/// drop notes (to conserve memory), but keep info about available tracks
	void dropNotes();
	/** Get formatted song label. **/
	std::string str() const { return title + "  by  " + artist; }
	/** Get full song information (used by the search function). **/
	std::string strFull() const { return title + "\n" + artist + "\n" + genre + "\n" + edition + "\n" + path; }
	/// Is the song parsed from the file yet?
	enum LoadStatus { NONE, HEADER, FULL } loadStatus;
	/// status of song
	enum Status { NORMAL, INSTRUMENTAL_BREAK, FINISHED };
	/** Get the song status at a given timestamp **/
	Status status(double time) const;
	int randomIdx; ///< sorting index used for random order
	VocalTrack vocals; ///< notes for the sing part
	InstrumentTracks instrumentTracks; ///< guitar etc. notes for this song
	DanceTracks danceTracks; ///< dance tracks
	bool hasDance() const { return !danceTracks.empty(); }
	bool hasDrums() const { return instrumentTracks.find(TrackName::DRUMS) != instrumentTracks.end(); }
	bool hasGuitars() const { return instrumentTracks.size() - hasDrums(); }
	bool hasVocals() const { return !vocals.notes.empty(); }
	std::string path; ///< path of songfile
	std::string filename; ///< name of songfile
	std::string midifilename; ///< name of midi file in FoF format
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

	typedef std::vector<std::pair<double,double> > Stops;
	Stops stops; ///< related to dance
	typedef std::vector<double> Beats;
	Beats beats; ///< related to instrument and dance
	bool hasBRE; ///< is there a Big Rock Ending? (used for drums only)
	bool b0rkedTracks; ///< are some tracks broken? (so that user can be notified)
	struct SongSection {
		std::string name;
		double begin;
		SongSection(std::string const& name, const double begin): name(name), begin(begin) {}
	};
	typedef std::vector<SongSection> SongSections;
	SongSections songsections; ///< vector of song sections
	bool getNextSection(double pos, SongSection &section);
	bool getPrevSection(double pos, SongSection &section);
};

static inline bool operator<(Song const& l, Song const& r) { return l.collateByArtist < r.collateByArtist; }

