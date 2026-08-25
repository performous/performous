#include "songparser-ini.hh"

#include "i18n.hh"
#include "songparser-mid.hh"
#include "songparserutil.hh"

#include "fs.hh"
#include "unicode.hh"
#include "util.hh"

#include <fmt/format.h>

#include <regex>
#include <stdexcept>
/// @file
/// Functions used for parsing the Frets on Fire INI song format

using namespace SongParserUtil;

namespace {
	const auto regex_icase = std::regex::icase;

	// There is some weird bug with std::regex and boost::locale on libc++ that makes regex fail if a global locale with a collation facet has been installed before instantiating patterns.

	const std::regex iniParseLine(
		R"(^[^\S^\r\n]*)"                                       // Any number of white-space characters that are neither \n nor \r
		R"(([a-zA-Z0-9._-]+))"                                  // INI key is one or more characters, letters and numbers, plus '.', '_' and '-'
		R"([^\S^\r\n]*)"                                        // Any number of white-space characters that are neither \n nor \r
		R"(=)"                                                  // Delimiter
		R"([^\S^\r\n]*)"                                        // Any number of white-space characters that are neither \n nor \r
		R"(([^\n\r]*?))"                                        // Non-greedy matching any character that is neither \r nor \n, and
		R"((?=[^\S^\r\n]*$))"                                   // That is followed by any number of white-space characters that are neither \n nor \r, and the end of the line.
	);

	const std::regex iniCheckHeader(
		R"((?:^|[\r\n]))"                                       // Start of the file, or right after a line break
		R"([^\S\r\n]*)"                                         // Any number of white-space characters that are neither \n nor \r
		R"(\[song\])"                                           // literal matching of [song]
		R"([^\S\r\n]*)"                                         // Any number of white-space characters that are neither \n nor \r
		R"((?:$|[\r\n;#]))", regex_icase                        // End of file, a line break, or a trailing comment
	);

	const std::regex richTags(
		R"(</?)"                                                // A '<', followed by either 0 or 1 slashes.
		R"((b|i|u|s|size|font|align|gradient|sub|sup|link))"    // Any one of these tags
		R"((=[^>]*)?)"                                          // A group of: '=' followed by any characters that are not >, appearing just 0 or 1 times as a whole.
		R"(( [^>]*)?>)"                                         // A group of: ' ' followed by any characters that are not >, appearing just 0 or 1 times as a whole, and finishing with >
		R"(|<color(=[^>]*)?>)"                                  // OR a <color> tag with an equal sign followed by anything that isn't '>'
		R"(|</color>)", regex_icase                             // OR the closing </color> tag.
	);

	const std::regex brTag(
		R"(<br>|<br[ ]*/?>)", regex_icase                       // match <br>, <br/> or <br />, allowing for any number of spaces between br and the /.
	);
}

IniSongParser::IniSongParser(std::string content) : m_ss(std::move(content)) {}

bool IniSongParser::getline(std::string& line) { return SongParserUtil::getLine(m_ss, line, m_linenum); }

void IniSongParser::parse(Song& song) {
	try {
		SongParserUtil::parseSong(song,
			[this](Song& s) { iniParseHeader(s); },
			[](Song& s) { SongParserMidi::parseNotes(s); SongParserUtil::finalize(s, 0, 0, 0.0); });
	}
	catch (SongParserException&) {
		throw;
	}
	catch (std::exception& e) {
		throw SongParserException(song, fmt::format("Caught exception={}", e.what()), m_linenum, false);
	}
}

/// 'Magick' to check if this file looks like correct format
bool IniSongParser::check(std::string const& data) {
	return std::regex_search(data.substr(0,1024), iniCheckHeader);
}

/// Parse header data for Songs screen
void IniSongParser::iniParseHeader(Song& song) {
	if (!song.vocalTracks.empty()) {
		song.vocalTracks.clear();
	}
	if (!song.instrumentTracks.empty()) {
		song.instrumentTracks.clear();
	}
	std::string line;

	while (getline(line)) {
		if (line.empty()) continue;
		if (trim(line)[0] == '[') { // Section header.
			if (UnicodeUtil::toLower(line).find("[song]") != std::string::npos) continue;
			break; // Keys should be under the correct section.
		}
		if ((line[0] == ';' || line[0] == '#') && line[1] == ' ') continue; // Comment.
		std::string key;
		std::string value;
		std::smatch match;
		if (std::regex_search(line, match, iniParseLine)) {
			key = UnicodeUtil::toLower(match[1].str());
			value = match[2].str();
		}
		// Strip rich-text tags.
		if (value.find("<") != std::string::npos) {
			// Step 1: Replace <br> with \n
			value = std::regex_replace(value, brTag, "\n");
			// Step 2: Remove explicitly listed tags.
			value = std::regex_replace(value, richTags, "");
		}
		if (trim(value).empty()) continue;
		// Supported tags
		if (key == "cassettecolor") continue; // Ignore.
		if (key == "name") song.title = value;
		else if (key == "artist") song.artist = value;
		else if (key == "cover") song.cover = absolute(value, song.path);
		else if (key == "background") song.background = absolute(value, song.path);
		else if (key == "video") song.video = absolute(value, song.path);
		else if (key == "genre") song.genre = value;
		else if (key == "frets") song.creator = value;
		else if (key == "delay") { assign(song.start, value); song.start/=1000.0; }
		else if (key == "video_start_time") { assign(song.videoGap, value); song.videoGap/=1000.0; }
		else if (key == "preview_start_time") { assign(song.preview_start, value); song.preview_start/=1000.0; }
		// Before adding other tags: they should be checked with the already-existing tags in FoF format; in case any tag doesn't exist there, it should be discussed with FoFiX developers before adding it here.
	}
	if (song.title.empty() || song.artist.empty()) {
		throw std::runtime_error("Required header fields missing");
	}
}
