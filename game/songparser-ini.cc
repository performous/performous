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
void SongParser::iniParseHeader(Song& song) {
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
