#include "songparser.hh"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>

/// @file
/// Functions used for parsing the UltraStar TXT song format

namespace {
	void assign(int& var, std::string const& str) {
		try {
			var = boost::lexical_cast<int>(str);
		} catch (...) {
			throw std::runtime_error("\"" + str + "\" is not valid integer value");
		}
	}
	void assign(double& var, std::string str) {
		std::replace(str.begin(), str.end(), ',', '.'); // Fix decimal separators
		try {
			var = boost::lexical_cast<double>(str);
		} catch (...) {
			throw std::runtime_error("\"" + str + "\" is not valid floating point value");
		}
	}
	void assign(bool& var, std::string const& str) {
		if (str == "YES" || str == "yes" || str == "1") var = true;
		else if (str == "NO" || str == "no" || str == "0") var = false;
		else throw std::runtime_error("Invalid boolean value: " + str);
	}
}

bool SongParser::txtCheck(std::vector<char> const& data) { return data[0] == '#' && data[1] >= 'A' && data[1] <= 'Z'; }

void SongParser::txtParse() {
	Song& s = m_song;
	std::string line;
	while (getline(line) && txtParseField(line)) {}
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
	if (m_bpm != 0.0) addBPM(0, m_bpm);
	while (txtParseNote(line) && getline(line)) {}
	// Workaround for the terminating : 1 0 0 line, written by some converters
	if (!s.notes.empty() && s.notes.back().type != Note::SLEEP && s.notes.back().begin == s.notes.back().end) s.notes.pop_back();
}

bool SongParser::txtParseField(std::string const& line) {
	if (line.empty()) return true;
	if (line[0] != '#') return false;
	std::string::size_type pos = line.find(':');
	if (pos == std::string::npos) throw std::runtime_error("Invalid format, should be #key:value");
	std::string key = boost::trim_copy(line.substr(1, pos - 1));
	std::string value = boost::trim_copy(line.substr(pos + 1));
	if (value.empty()) return true;
	if (key == "TITLE") m_song.title = value.substr(value.find_first_not_of(" :"));
	else if (key == "ARTIST") m_song.artist = value.substr(value.find_first_not_of(" "));
	else if (key == "EDITION") m_song.edition = value.substr(value.find_first_not_of(" "));
	else if (key == "GENRE") m_song.genre = value.substr(value.find_first_not_of(" "));
	else if (key == "CREATOR") m_song.creator = value.substr(value.find_first_not_of(" "));
	else if (key == "COVER") m_song.cover = value;
	else if (key == "MP3") m_song.music["background"] = m_song.path + value;
	else if (key == "VOCALS") m_song.music["vocals"] = m_song.path + value;
	else if (key == "VIDEO") m_song.video = value;
	else if (key == "BACKGROUND") m_song.background = value;
	else if (key == "START") assign(m_song.start, value);
	else if (key == "VIDEOGAP") assign(m_song.videoGap, value);
	else if (key == "RELATIVE") assign(m_relative, value);
	else if (key == "GAP") { assign(m_gap, value); m_gap *= 1e-3; }
	else if (key == "BPM") assign(m_bpm, value);
	else if (key == "LANGUAGE") m_song.language= value.substr(value.find_first_not_of(" "));
	return true;
}

bool SongParser::txtParseNote(std::string line) {
	if (line.empty() || line == "\r") return true;
	if (line[0] == '#') throw std::runtime_error("Key found in the middle of notes");
	if (line[line.size() - 1] == '\r') line.erase(line.size() - 1);
	if (line[0] == 'E') return false;
	std::istringstream iss(line);
	if (line[0] == 'B') {
		unsigned int ts;
		double bpm;
		iss.ignore();
		if (!(iss >> ts >> bpm)) throw std::runtime_error("Invalid BPM line format");
		addBPM(ts, bpm);
		return true;
	}
	Note n;
	n.type = Note::Type(iss.get());
	unsigned int ts = m_prevts;
	switch (n.type) {
	  case Note::NORMAL:
	  case Note::FREESTYLE:
	  case Note::GOLDEN:
		{
			unsigned int length = 0;
			if (!(iss >> ts >> length >> n.note)) throw std::runtime_error("Invalid note line format");
			n.notePrev = n.note; // No slide notes in TXT yet.
			if (m_relative) ts += m_relativeShift;
			if (iss.get() == ' ') std::getline(iss, n.syllable);
			n.end = tsTime(ts + length);
		}
		break;
	  case Note::SLEEP:
		{
			unsigned int end;
			if (!(iss >> ts >> end)) end = ts;
			if (m_relative) {
				ts += m_relativeShift;
				end += m_relativeShift;
				m_relativeShift = end;
			}
			n.end = tsTime(end);
		}
		break;
	  default: throw std::runtime_error("Unknown note type");
	}
	n.begin = tsTime(ts);
	Notes& notes = m_song.notes;
	if (m_relative && m_song.notes.empty()) m_relativeShift = ts;
	m_prevts = ts;
	if (n.begin < m_prevtime) {
		// Oh no, overlapping notes (b0rked file)
		// Can't do this because too many songs are b0rked: throw std::runtime_error("Note overlaps with previous note");
		if (notes.size() >= 1) {
			Note& p = notes.back();
			// Workaround for songs that use semi-random timestamps for sleep
			if (p.type == Note::SLEEP) {
				p.end = p.begin;
				Notes::reverse_iterator it = notes.rbegin();
				Note& p2 = *++it;
				if (p2.end < n.begin) p.begin = p.end = n.begin;
			}
			// Can we just make the previous note shorter?
			if (p.begin <= n.begin) p.end = n.begin;
			else throw std::runtime_error("Note overlaps with earlier notes");
		} else throw std::runtime_error("The first note has negative timestamp");
	}
	double prevtime = m_prevtime;
	m_prevtime = n.end;
	if (n.type != Note::SLEEP && n.end > n.begin) {
		m_song.noteMin = std::min(m_song.noteMin, n.note);
		m_song.noteMax = std::max(m_song.noteMax, n.note);
		m_maxScore += n.maxScore();
	}
	if (n.type == Note::SLEEP) {
		if (notes.empty()) return true; // Ignore sleeps at song beginning
		n.begin = n.end = prevtime; // Normalize sleep notes
	}
	notes.push_back(n);
	return true;
}

