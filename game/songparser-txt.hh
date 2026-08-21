#pragma once

#include "isongparser.hh"
#include "song.hh"

#include <sstream>
#include <string>

/// Parses the UltraStar TXT song format.
class TxtSongParser : public ISongParser {
public:
	explicit TxtSongParser(std::string content);
	void parse(Song&) override;

	/// 'Magick' to check if the data looks like this format
	static bool check(std::string const& data);

private:
	void txtParseHeader(Song&);
	void txtParse(Song&);
	bool txtParseField(Song&, std::string const& line);
	bool txtParseNote(Song&, std::string line);
	void txtResetState(Song&);
	bool getline(std::string& line);

	std::stringstream m_ss;
	unsigned m_linenum = 0;
	bool m_relative = false;
	double m_gap = 0.0;
	float m_bpm = 0.0f;
	enum class CurrentSinger { P1, P2, BOTH } m_curSinger = CurrentSinger::P1;
	/// The following struct is cleared between tracks
	struct TXTState {
		double prevtime = 0.0;
		unsigned prevts = 0;
		unsigned relativeShift = 0;
	} m_txt;
};
