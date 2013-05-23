#include "songparser.hh"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <stdexcept>
#include "midifile.hh"

/// @file
/// Functions used for parsing the Frets on Fire INI song format

using namespace SongParserUtil;

/// 'Magick' to check if this file looks like correct format
bool SongParser::iniCheck(std::vector<char> const& data) const {
	static const std::string header = "[song]";
	return std::equal(header.begin(), header.end(), data.begin());
}

/// Parse header data for Songs screen
void SongParser::iniParseHeader() {
	Song& s = m_song;
	std::string line;
	while (getline(line)) {
		if (line.empty()) continue;
		if (line[0] == '[') continue; // Section header
		std::istringstream iss(line);
		std::string key, value;
		if (!std::getline(iss, key, '=') || !std::getline(iss, value)) std::runtime_error("Invalid format, should be key=value");
		boost::trim(key); boost::to_lower(key);
		boost::trim(value);
		// Supported tags
		if (key == "name") s.title = value;
		else if (key == "artist") s.artist = value;
		else if (key == "cover") s.cover = value;
		else if (key == "background") s.background = value;
		else if (key == "video") s.video = value;
		else if (key == "genre") s.genre = value;
		else if (key == "frets") s.creator = value;
		else if (key == "delay") { assign(s.start, value); s.start/=1000.0; }
		else if (key == "video_start_time") { assign(s.videoGap, value); s.videoGap/=1000.0; }
		else if (key == "preview_start_time") { assign(s.preview_start, value); s.preview_start/=1000.0; }
		// Before adding other tags: they should be checked with the already-existing tags in FoF format; in case any tag doesn't exist there, it should be discussed with FoFiX developers before adding it here.
	}
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");

	// Parse additional data from midi file - required to get tracks info
	s.midifilename = "notes.mid";
	// Compose regexps to find music files
	boost::regex midifile("(.*\\.mid)$", boost::regex_constants::icase);
	boost::regex audiofile_background("(song\\.ogg)$", boost::regex_constants::icase);
	boost::regex audiofile_guitar("(guitar\\.ogg)$", boost::regex_constants::icase);
	boost::regex audiofile_drums("(drums\\.ogg)$", boost::regex_constants::icase);
	boost::regex audiofile_bass("(rhythm\\.ogg)$", boost::regex_constants::icase);
	boost::regex audiofile_keyboard("(keyboard\\.ogg)$", boost::regex_constants::icase);
	boost::regex audiofile_vocals("(vocals\\.ogg)$", boost::regex_constants::icase);
	boost::regex audiofile_other("(.*\\.ogg)$", boost::regex_constants::icase);
	boost::cmatch match;

	midParseHeader();
}

/// Parse notes
void SongParser::iniParse() {
	midParse();
}


