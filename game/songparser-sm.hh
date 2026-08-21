#pragma once

#include "isongparser.hh"
#include "song.hh"

#include <sstream>
#include <string>
#include <utility>

/// Parses the StepMania SM format.
class SmSongParser : public ISongParser {
public:
	explicit SmSongParser(std::string content);
	void parse(Song&) override;

	/// 'Magick' to check if the data looks like this format
	static bool check(std::string const& data);

private:
	/// Parses the entire file, including notes; parse() drops the notes again for a header-only pass.
	void smParseHeader(Song&);
	bool smParseField(Song&, std::string line);
	Notes smParseNotes(Song&, std::string line);
	std::pair<double, double> smStopConvert(Song&, std::pair<double, double> s);
	bool getline(std::string& line);

	std::stringstream m_ss;
	unsigned m_linenum = 0;
	double m_gap = 0.0;
	float m_bpm = 0.0f;
	unsigned m_tsPerBeat = 0;  ///< The ts increment per beat
	unsigned m_tsEnd = 0;  ///< The ending ts of the song
	Song::Stops m_stops;  ///< Stops stored in <ts, duration> format
};
