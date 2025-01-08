#pragma once

#include "libxml++.hh"
#include "song.hh"
#include "unicode.hh"
#include "fs.hh"
#include "isongparser.hh"

#include <cstdint>
#include <boost/range/adaptor/reversed.hpp>
#include <sstream>

namespace SongParserUtil {
	const std::string DUET_P2 = "Duet singer";	// FIXME
	const std::string DUET_BOTH = "Both singers";	// FIXME
	/// Parse an int from string and assign it to a variable
	void assign(int& var, std::string const& str);
	/// Parse an unsigned int from string and assign it to a variable
	void assign(unsigned& var, std::string const& str);
	/// Parse a double from string and assign it to a variable
	void assign(double& var, std::string str);
	/// Parse a float from string and assign it to a variable
	void assign(float& var, std::string str);
	/// Parse a boolean from string and assign it to a variable
	void assign(bool& var, std::string const& str);
	/// Erase last character if it matches
	void eraseLast(std::string& s, char ch = ' ');
}

/// Parse a song file; this object is only used while parsing and is discarded once done.
/// Format-specific member functions are implemented in songparser-*.cc.
class SongParser : public ISongParser {
public:
	/// Parse into s
	void parse(Song & s) override;

private:
	// Variables and types
	std::stringstream m_ss;
	unsigned m_linenum = 0;
	bool m_relative = false;
	double m_gap = 0.0;
	float m_bpm = 0.0f;
	unsigned m_tsPerBeat = 0;  ///< The ts increment per beat
	unsigned m_tsEnd = 0;  ///< The ending ts of the song
	enum class CurrentSinger { P1, P2, BOTH } m_curSinger = CurrentSinger::P1;
	Song::Stops m_stops;  ///< Stops stored in <ts, duration> format
	/// The following struct is cleared between tracks
	struct TXTState {
		double prevtime = 0.0;
		unsigned prevts = 0;
		unsigned relativeShift = 0;
	} m_txt;
	// Functions
	void finalize(Song&);
	void vocalsTogether(Song&);
	void guessFiles(Song&);
	bool getline (std::string& line) { ++m_linenum; return (bool) std::getline (m_ss, line); }
	Song::BPM getBPM(Song const& s, double ts) const;
	void addBPM(Song&, double ts, float bpm);
	double tsTime(Song&, double ts) const;  ///< Convert a timestamp (beats) into time (seconds)
	bool txtCheck(std::string const& data) const;
	void txtParseHeader(Song&);
	void txtParse(Song&);
	bool txtParseField(Song&, std::string const& line);
	bool txtParseNote(Song&, std::string line);
	void txtResetState(Song&);
	bool iniCheck(std::string const& data) const;
	void iniParseHeader(Song&);
	bool midCheck(std::string const& data) const;
	void midParseHeader(Song&);
	void midParse(Song&);
	bool xmlCheck(std::string const& data) const;
	void xmlParseHeader(Song&);
	void xmlParse(Song&);
	Note xmlParseNote(Song&, xmlpp::Element const& noteNode, unsigned& ts);
	bool smCheck(std::string const& data) const;
	void smParseHeader(Song&);
	void smParse(Song&);
	bool smParseField(Song&, std::string line);
	Notes smParseNotes(Song&, std::string line);
	std::pair<double, double> smStopConvert(Song&, std::pair<double, double> s);
};
