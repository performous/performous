#include "songparser.hh"

#include "midifile.hh"

/// @file Functions used for parsing the UltraStar TXT song format

bool SongParser::iniCheck(std::vector<char> const& data) {
	static const std::string header = "[song]";
	return std::equal(header.begin(), header.end(), data.begin());
}

namespace {
	void eraseLast(std::string& s, char ch = ' ') {
		if (!s.empty() && *s.rbegin() == ch) s.erase(s.size() - 1);
	}
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
		boost::trim(key); boost::to_lower(key);
		boost::trim(value);
		if (key == "name") s.title = value;
		else if (key == "artist") s.artist = value;
	}
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
	s.mp3 = "song.ogg";
	MidiFileParser midi(s.path + "/notes.mid");
	for (MidiFileParser::TempoChanges::const_iterator it = midi.tempochanges.begin(); it != midi.tempochanges.end(); ++it) addBPM(it->miditime * 4, it->value);
	for (MidiFileParser::Tracks::const_iterator it = midi.tracks.begin(); it != midi.tracks.end(); ++it) {
		if (it->name != "PART VOCALS") continue;
		for (MidiFileParser::Lyrics::const_iterator it2 = it->lyrics.begin(); it2 != it->lyrics.end(); ++it2) {
			Note n;
			n.begin = midi.get_seconds(it2->begin);
			n.end = midi.get_seconds(it2->end);
			n.note = it2->note;
			n.type = n.note > 100 ? Note::SLEEP : Note::NORMAL;
			n.syllable = it2->lyric;
			if (!n.syllable.empty()) {
				switch (*n.syllable.rbegin()) {
				  case '-': eraseLast(n.syllable, '-'); break;
				  case '+': eraseLast(s.notes.back().syllable); *n.syllable.rbegin() = '~'; break;
				  default: n.syllable += " "; break;
				}
			}
			if (n.type == Note::NORMAL) {
				s.noteMin = std::min(s.noteMin, n.note);
				s.noteMax = std::max(s.noteMax, n.note);
				s.notes.push_back(n);
			} else if (!s.notes.empty() && s.notes.back().type != Note::SLEEP) {
				eraseLast(s.notes.back().syllable);
				s.notes.push_back(n);
			}
		}
		if (!s.notes.empty()) break;
	}
	if (s.notes.empty()) {
		Note n;
		n.begin = 30.0;
		n.end = 31.0;
		n.syllable = "TODO";
		s.notes.push_back(n);
	}
}


