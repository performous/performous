#pragma once

#include "libxml++.hh"
#include "song.hh"
#include "songparserutil.hh"
#include "unicode.hh"
#include "fs.hh"
#include "isongparser.hh"

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

	const auto regex_icase = std::regex::icase;


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
		R"((?:$|[;#]))", regex_multiline | regex_icase           // Non-capturing group; match end-of-line or either ';' or '#', which denote a trailing comment.
	);

	const static std::regex richTags(
		R"(</?)"                                                // A '<', followed by either 0 or 1 slashes.
		R"((b|i|u|s|size|font|align|gradient|sub|sup|link))"    // Any one of these tags
		R"((=[^>]*)?)"                                          // A group of: '=' followed by any characters that are not >, appearing just 0 or 1 times as a whole.
		R"(( [^>]*)?>)"                                         // A group of: ' ' followed by any characters that are not >, appearing just 0 or 1 times as a whole, and finishing with >
		R"(|<color(=[^>]*)?>)"                                  // OR a <color> tag with an equal sign followed by anything that isn't '>'
		R"(|</color>)", regex_icase                             // OR the closing </color> tag.
	);

	const static std::regex brTag(
		R"(<br>|<br[ ]*/?>)", regex_icase                       // match <br>, <br/> or <br />, allowing for any number of spaces between br and the /.
	);
}

/// Parse a song file; this object is only used while parsing and is discarded once done.
/// Format-specific member functions are implemented in songparser-*.cc.
class SongParser : public ISongParser {
public:
	void parse(Song&) override;

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
	bool getline(std::string& line) { return SongParserUtil::getLine(m_ss, line, m_linenum); }
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
