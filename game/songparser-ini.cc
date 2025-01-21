#include "songparser.hh"

#include "i18n.hh"
#include "fs.hh"
#include "midifile.hh"
#include "unicode.hh"
#include "util.hh"

#include <regex>
#include <stdexcept>
/// @file
/// Functions used for parsing the Frets on Fire INI song format

using namespace SongParserUtil;

/// 'Magick' to check if this file looks like correct format
bool SongParser::iniCheck(std::string const& data) const {
	return std::regex_search(data.substr(0,1024), iniCheckHeader);
}

/// Parse header data for Songs screen
void SongParser::iniParseHeader() {
	Song& s = m_song;
	if (!m_song.vocalTracks.empty()) { m_song.vocalTracks.clear(); }
	if (!m_song.instrumentTracks.empty()) { m_song.instrumentTracks.clear(); }
	std::string line;
	
	while (getline(line)) {
		if (line.empty()) continue;
		if (trim(line)[0] == '[') { // Section header.
			if (line.find("[song]") != std::string::npos) continue;
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
		if (key == "name") s.title = value;
		else if (key == "artist") s.artist = value;
		else if (key == "cover") s.cover = absolute(value, s.path);
		else if (key == "background") s.background = absolute(value, s.path);
		else if (key == "video") s.video = absolute(value, s.path);
		else if (key == "genre") s.genre = value;
		else if (key == "frets") s.creator = value;
		else if (key == "delay") { assign(s.start, value); s.start/=1000.0; }
		else if (key == "video_start_time") { assign(s.videoGap, value); s.videoGap/=1000.0; }
		else if (key == "preview_start_time") { assign(s.preview_start, value); s.preview_start/=1000.0; }
		// Before adding other tags: they should be checked with the already-existing tags in FoF format; in case any tag doesn't exist there, it should be discussed with FoFiX developers before adding it here.
	}
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
}

