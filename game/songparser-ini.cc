#include "songparser.hh"
#include "fs.hh"

#include "unicode.hh"
#include <boost/algorithm/string.hpp>
#include <stdexcept>
#include "midifile.hh"

/// @file
/// Functions used for parsing the Frets on Fire INI song format

using namespace SongParserUtil;

/// 'Magick' to check if this file looks like correct format
bool SongParser::iniCheck(std::string const& data) const {
	static const std::string header = "[song]";
	return std::equal(header.begin(), header.end(), data.begin());
}

/// Parse header data for Songs screen
void SongParser::iniParseHeader() {
	Song& s = m_song;
	if (!m_song.vocalTracks.empty()) { m_song.vocalTracks.clear(); }
	if (!m_song.instrumentTracks.empty()) { m_song.instrumentTracks.clear(); }
	std::string line;
	while (getline(line)) {
		if (line.empty()) continue;
		if (line[0] == '[') continue; // Section header
		std::istringstream iss(line);
		std::string key, value;
		if (!std::getline(iss, key, '=') || !std::getline(iss, value)) std::runtime_error("Invalid format, should be key=value");
		boost::trim(key); key = UnicodeUtil::toLower(key);
		boost::trim(value);
		// Supported tags
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

