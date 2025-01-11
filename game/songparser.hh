#pragma once

#include "libxml++.hh"
#include "song.hh"
#include "unicode.hh"
#include "fs.hh"

#include <boost/range/adaptor/reversed.hpp>

#include <cstdint>
#include <regex>
#include <sstream>


namespace SongParserUtil {

#if defined(_MSC_VER)
	const auto regex_multiline = std::regex_constants::ECMAScript; // MSVC hasn't implemented multiline.
#else
	const auto regex_multiline = std::regex::multiline;
#endif

	// There is some weird bug with std::regex and boost::locale on libc++ that makes regex fail if a global locale with a collation facet has been installed before instantiating patterns.

	const static std::regex iniParseLine(
		R"(^[^\S^\r\n]*)"                                       // Any number of white-space characters that are neither \n nor \r
		R"(([a-zA-Z0-9._-]+))"                                  // INI key is one or more characters, letters and numbers, plus '.', '_' and '-'
		R"([^\S^\r\n]*)"                                        // Any number of white-space characters that are neither \n nor \r
		R"(=)"                                                  // Delimiter
		R"([^\S^\r\n]*)"                                        // Any number of white-space characters that are neither \n nor \r
		R"(([^\n\r]*?))"                                        // Non-greedy matching any character that is neither \r nor \n, and
		R"((?=[^\S^\r\n]*$))", regex_multiline                  // That is followed by any number of white-space characters that are neither \n nor \r, and the end of the line.
	);

	const static std::regex iniCheckHeader(
		R"(^[^\S^\r\n]*)"                                       // Any number of white-space characters that are neither \n nor \r
		R"(\[song\])"                                           // literal matching of [song]
		R"([^\S^\r\n]*)"                                        // Any number of white-space characters that are neither \n nor \r
		R"((?:$|[;#]))", regex_multiline                        // Non-capturing group; match end-of-line or either ';' or '#', which denote a trailing comment.
	);

	const static std::regex richTags(
		R"(</?)"                                                // A '<', followed by either 0 or 1 slashes.
		R"((b|i|u|s|size|font|align|gradient|sub|sup|link))"    // Any one of these tags
		R"((=[^>]*)?)"                                          // A group of: '=' followed by any characters that are not >, appearing just 0 or 1 times as a whole.
		R"(( [^>]*)?>)"                                         // A group of: ' ' followed by any characters that are not >, appearing just 0 or 1 times as a whole, and finishing with >
		R"(|<color(=[^>]*)?>)"                                  // OR a <color> tag with an equal sign followed by anything that isn't '>'
		R"(|</color>)", std::regex_constants::icase             // OR the closing </color> tag. 
	);

	const static std::regex brTag(
		R"(<br>|<br[ ]*/?>)", std::regex_constants::icase       // match <br>, <br/> or <br />, allowing for any number of spaces between br and the /.
	);

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
class SongParser {
public:
	/// Parse into s
	SongParser (Song & s);
private:
	// Variables and types
	Song& m_song;
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
	void finalize();
	void vocalsTogether();
	void guessFiles();
	bool getline (std::string& line) { ++m_linenum; return (bool) std::getline (m_ss, line); }
	Song::BPM getBPM(Song const& s, double ts) const;
	void addBPM(double ts, float bpm);
	double tsTime(double ts) const;	 ///< Convert a timestamp (beats) into time (seconds)
	bool txtCheck(std::string const& data) const;
	void txtParseHeader();
	void txtParse();
	bool txtParseField(std::string const& line);
	bool txtParseNote(std::string line);
	void txtResetState();
	bool iniCheck(std::string const& data) const;
	void iniParseHeader();
	bool midCheck(std::string const& data) const;
	void midParseHeader();
	void midParse();
	bool xmlCheck(std::string const& data) const;
	void xmlParseHeader();
	void xmlParse();
	Note xmlParseNote(xmlpp::Element const& noteNode, unsigned& ts);
	bool smCheck(std::string const& data) const;
	void smParseHeader();
	void smParse();
	bool smParseField(std::string line);
	Notes smParseNotes(std::string line);
	std::pair<double, double> smStopConvert(std::pair<double, double> s);
};
