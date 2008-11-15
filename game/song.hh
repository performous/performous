#pragma once
#ifndef PERFORMOUS_SONG_HH
#define PERFORMOUS_SONG_HH

#include "notes.hh"
#include <boost/noncopyable.hpp>
#include <stdexcept>

struct SongParserException: public std::runtime_error {
	SongParserException(std::string const& msg, unsigned int linenum): runtime_error(msg), m_linenum(linenum) {}
	unsigned int line() const { return m_linenum; }
  private:
	unsigned int m_linenum;
};

class SongParser;

class Song: boost::noncopyable {
  public:
	friend class SongParser;
	Song(std::string const& path, std::string const& filename);
	void reload();
	bool parseField(std::string const& line);
	/** Get formatted song label. **/
	std::string str() const { return title + "  by  " + artist; }
	/** Get full song information (used by the search function). **/
	std::string strFull() const { return title + "\n" + artist + "\n" + genre + "\n" + edition + "\n" + path; }
	Notes notes;
	int noteMin, noteMax;
	std::string path;
	std::string filename;
	std::vector<std::string> category;
	std::string genre;
	std::string edition;
	std::string title;
	std::string artist;
	std::string text;
	std::string creator;
	std::string mp3;
	std::string cover;
	std::string background;
	std::string video;
	double videoGap;
	double start;
	MusicalScale scale;
	std::vector<double> timePitchGraph;
	std::vector<double> pitchPitchGraph;
	std::vector<double> volumePitchGraph;
	std::vector<bool> drawPitchGraph;
	double m_scoreFactor; // Normalization factor for the scoring system
  private:
};

bool operator<(Song const& l, Song const& r);

#endif
