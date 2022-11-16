#pragma once

#include "fs.hh"
#include "i18n.hh"
#include "json.hh"
#include "notes.hh"
#include "util.hh"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

class SongParser;

namespace TrackName {
	const std::string BGMUSIC = "background";
	const std::string PREVIEW = "Preview";
	const std::string GUITAR = "Guitar";
	const std::string GUITAR_COOP = "Coop guitar";
	const std::string GUITAR_RHYTHM = "Rhythm guitar";
	const std::string BASS = "Bass";
	const std::string KEYBOARD = "Keyboard";
	const std::string DRUMS = "Drums";
	const std::string DRUMS_SNARE = "Drum snare";
	const std::string DRUMS_CYMBALS = "Drum cymbals";
	const std::string DRUMS_TOMS = "Drum toms";
	const std::string LEAD_VOCAL = "Vocals";
	const std::string BACKING_VOCAL = "Vocals background";
	#if 0 // Here is some dummy gettext calls to populate the dictionary
	_("Guitar") _("Coop guitar") _("Rhythm guitar") _("Bass") _("Drums") _("Vocals")  _("Harmonic 1") _("Harmonic 2") _("Harmonic 3")
	#endif
}

class ScreenSing;

/// Song object contains all information about a song (headers, notes)
class Song {
	friend class SongParser;
public:
	/// Is the song parsed from the file yet?
	enum class LoadStatus { NONE, HEADER, FULL } loadStatus = LoadStatus::NONE;
	/// status of song
	enum class Status { NORMAL, INSTRUMENTAL_BREAK, FINISHED };
	VocalTracks vocalTracks; ///< notes for the sing part
	VocalTrack dummyVocal; ///< notes for the sing part
	InstrumentTracks instrumentTracks; ///< guitar etc. notes for this song
	DanceTracks danceTracks; ///< dance tracks
	fs::path path; ///< path of songfile
	fs::path filename; ///< name of songfile
	fs::path midifilename; ///< name of midi file in FoF format
	struct BPM {
		BPM (double _begin, double _ts, float bpm) :
		begin (_begin), step (0.25 * 60.0 / bpm), ts (_ts) {}
		double begin;  // Time in seconds
		double step;  // Seconds per quarter note
		double ts;
	};
	std::vector<BPM> m_bpms;
	std::vector<std::string> category; ///< category of song
	std::string genre; ///< genre
	std::string edition; ///< license
	std::string title; ///< songtitle
	std::string artist; ///< artist
	std::string text; ///< songtext
	std::string creator; ///< creator
	std::string language; ///< language
	using MusicFiles = std::map<std::string, fs::path>;
	MusicFiles music; ///< music files (background, guitar, rhythm/bass, drums, vocals)
	fs::path cover; ///< cd cover
	fs::path background; ///< background image
	fs::path video; ///< video
	std::string collateByTitle;  ///< String for sorting by title, artist
	std::string collateByTitleOnly;  ///< String for sorting by title only
	std::string collateByArtist;  ///< String for sorting by artist, title
	std::string collateByArtistOnly;  ///< String for sorting by artist only
	double videoGap = 0.0; ///< gap with video
	double start = 0.0; ///< start of song
	double preview_start = getNaN(); ///< starting time for the preview
	double m_duration = 0.0;
	using Stops = std::vector<std::pair<double,double> >;
	Stops stops; ///< related to dance
	using Beats = std::vector<double>;
	Beats beats; ///< related to instrument and dance
	bool hasBRE = false; ///< is there a Big Rock Ending? (used for drums only)
	std::string b0rked; ///< Is something broken? (so that user can be notified)
	struct SongSection {
		std::string name;
		double begin;
		SongSection(std::string const& name, const double begin): name(name), begin(begin) {}
	};
	std::vector<SongSection> songsections; ///< vector of song sections
	int randomIdx = 0; ///< sorting index used for random order

	// Functions only below this line
	Song(nlohmann::json const& song);  ///< Load song from cache.
	Song(fs::path const& path, fs::path const& filename);  ///< Load song from specified path and filename
	void reload(bool errorIgnore = true);  ///< Reset and reload the entire song from file
	void loadNotes(bool errorIgnore = true);  ///< Load note data (called when entering singing screen, headers preloaded).
	void dropNotes();  ///< Remove note data (when exiting singing screen), to conserve RAM
	void insertVocalTrack(std::string vocalTrack, VocalTrack track);
	void eraseVocalTrack(std::string vocalTrack = TrackName::LEAD_VOCAL);
	std::string str() const;  ///< Return "title by artist" string for UI
	std::string strFull() const;  ///< Return multi-line full song info (used for searching)
	/** Get the song status at a given timestamp **/
	Status status(double time, ScreenSing* song);
	// Get a selected track, or LEAD_VOCAL if not found or the first one if not found
	VocalTrack& getVocalTrack(std::string vocalTrack = TrackName::LEAD_VOCAL);
	VocalTrack& getVocalTrack(unsigned idx = 0);
	std::vector<std::string> getVocalTrackNames() const;
	double getDurationSeconds();
	bool hasDance() const { return !danceTracks.empty(); }
	bool hasDrums() const { return instrumentTracks.find(TrackName::DRUMS) != instrumentTracks.end(); }
	bool hasKeyboard() const { return instrumentTracks.find(TrackName::KEYBOARD) != instrumentTracks.end(); }
	bool hasGuitars() const { return instrumentTracks.size() - hasDrums() - hasKeyboard(); }
	bool hasVocals() const { return !vocalTracks.empty(); }
	bool hasDuet() const { return vocalTracks.size() > 1; }
	bool hasControllers() const { return !danceTracks.empty() || !instrumentTracks.empty(); }
	bool getNextSection(double pos, SongSection &section);
	bool getPrevSection(double pos, SongSection &section);
private:
	void collateUpdate();   ///< Rebuild collate variables (used for sorting) from other strings
};

/// Thrown by SongParser when there is an error
struct SongParserException: public std::runtime_error {
	/// constructor
	SongParserException(Song& s, std::string const& msg, unsigned int linenum, bool sil = false): runtime_error(msg), m_filename(s.filename), m_linenum(linenum), m_silent(sil) {
		if (!sil) s.b0rked += msg + '\n';
	}
	~SongParserException() noexcept = default;
	fs::path const& file() const { return m_filename; } ///< file in which the error occured
	unsigned int line() const { return m_linenum; } ///< line in which the error occured
	bool silent() const { return m_silent; } ///< if the error should not be printed to user (file skipped)
private:
	fs::path m_filename;
	unsigned int m_linenum;
	bool m_silent;
};

using SongPtr = std::shared_ptr<Song>;

/// Print a SongParserException in a format suitable for the logging system.
std::ostream& operator<<(std::ostream& os, SongParserException const& e);
