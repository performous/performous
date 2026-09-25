#pragma once

#include "isongparser.hh"
#include "song.hh"

#include <sstream>
#include <string>

/// Parses the Frets on Fire INI song format. INI songs have no notes of their own — the note
/// data lives in a companion MIDI file referenced by the header (see SongParserMidi).
class IniSongParser : public ISongParser {
public:
	explicit IniSongParser(std::string content);
	void parse(Song&) override;

	/// 'Magick' to check if the data looks like this format
	static bool check(std::string const& data);

private:
	void iniParseHeader(Song&);
	bool getline(std::string& line);

	std::stringstream m_ss;
	unsigned m_linenum = 0;
};
