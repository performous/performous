#include "songparser.hh"

#include <boost/lexical_cast.hpp>
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

namespace {
	void testAndAdd(Song& s, std::string const& trackid, std::string const& filename) {
		std::string f = s.path + filename;
		if (boost::filesystem::exists(f)) s.music[trackid] = f;
	}
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

	// Search the dir for the music files
	for (boost::filesystem::directory_iterator dirIt(s.path), dirEnd; dirIt != dirEnd; ++dirIt) {
		boost::filesystem::path p = dirIt->path();
#if BOOST_FILESYSTEM_VERSION < 3
		std::string name = p.leaf(); // File basename (notes.txt)
#else
		std::string name = p.filename().string(); // File basename (notes.txt)
#endif
		if (regex_match(name.c_str(), match, midifile)) {
			 s.midifilename = name;
		} else if (regex_match(name.c_str(), match, audiofile_background)) {
			testAndAdd(s, "background", name);
		} else if (regex_match(name.c_str(), match, audiofile_guitar)) {
			testAndAdd(s, TrackName::GUITAR, name);
		} else if (regex_match(name.c_str(), match, audiofile_bass)) {
			testAndAdd(s, TrackName::BASS, name);
		} else if (regex_match(name.c_str(), match, audiofile_keyboard)) {
			testAndAdd(s, TrackName::KEYBOARD, name);
		} else if (regex_match(name.c_str(), match, audiofile_drums)) {
			testAndAdd(s, TrackName::DRUMS, name);
		} else if (regex_match(name.c_str(), match, audiofile_vocals)) {
			testAndAdd(s, "vocals", name);
#if 0  // TODO: process preview.ogg properly? In any case, do not print debug to console...
		} else if (regex_match(name.c_str(), match, audiofile_other)) {
			std::cout << "Found unknown ogg file: " << name << std::endl;
#endif
		}
	}
	midParseHeader();
}

/// Parse notes
void SongParser::iniParse() {
	midParse();
}


