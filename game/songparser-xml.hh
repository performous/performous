#pragma once

#include "isongparser.hh"
#include "song.hh"

#include <sstream>
#include <string>

namespace xmlpp { class Element; }

/// Parses the SingStar XML song format.
class XmlSongParser : public ISongParser {
public:
	explicit XmlSongParser(std::string content);
	void parse(Song&) override;

	/// 'Magick' to check if the data looks like this format
	static bool check(std::string const& data);

private:
	void xmlParseHeader(Song&);
	void xmlParse(Song&);
	Note xmlParseNote(Song&, xmlpp::Element const& noteNode, unsigned& ts);

	std::stringstream m_ss;
	float m_bpm = 0.0f;
};
