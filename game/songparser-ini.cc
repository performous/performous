#include "songparser.hh"

/// @file Functions used for parsing the UltraStar TXT song format

bool SongParser::iniCheck(std::vector<char> const& data) {
	static const std::string header = "[song]";
	return std::equal(header.begin(), header.end(), data.begin());
}

void SongParser::iniParse() {
	Song& s = m_song;
	std::string line;
	while (getline(line)) {
		if (line.empty()) continue;
		if (line[0] == '[') continue; // Section header
		std::istringstream iss(line);
		std::string key, value;
		if (!std::getline(iss, key, '=') || !std::getline(iss, value)) std::runtime_error("Invalid format, should be key=value");
		boost::trim(key);
		boost::trim(value);
		if (key == "name") s.title = value;
		else if (key == "artist") s.artist = value;
	}
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
	s.mp3 = "song.ogg";
	addBPM(0, 120);
	Note n;
	n.begin = 30.0;
	n.end = 31.0;
	n.syllable = "TODO";
	s.notes.push_back(n);
}

