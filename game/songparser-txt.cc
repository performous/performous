#include "songparser.hh"

#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <stdexcept>

/// @file
/// Functions used for parsing the UltraStar TXT song format

using namespace SongParserUtil;

namespace {
	const std::string DUET_P2 = "Duet singer"; // FIXME
}

/// 'Magick' to check if this file looks like correct format
bool SongParser::txtCheck(std::string const& data) const {
	return data[0] == '#' && data[1] >= 'A' && data[1] <= 'Z';
}

/// Parse header data for Songs screen
void SongParser::txtParseHeader() {
	Song& s = m_song;
	std::string line;
	s.insertVocalTrack(TrackName::LEAD_VOCAL, VocalTrack(TrackName::LEAD_VOCAL)); // Dummy note to indicate there is a track
	while (getline(line) && txtParseField(line)) {}
	if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
	if (m_bpm != 0.0) addBPM(0, m_bpm);
}

/// Parse notes
void SongParser::txtParse() {
	std::string line;
	m_curSinger = P1;
	m_song.insertVocalTrack(TrackName::LEAD_VOCAL, VocalTrack(TrackName::LEAD_VOCAL));
	m_song.insertVocalTrack(DUET_P2, VocalTrack(DUET_P2));
	while (getline(line) && txtParseField(line)) {} // Parse the header again
	resetNoteParsingState();
	while (txtParseNote(line) && getline(line)) {} // Parse notes

	// Workaround for the terminating : 1 0 0 line, written by some converters
	// FIXME: Should we do this for all tracks?
	for (auto const& name: { TrackName::LEAD_VOCAL, DUET_P2 }) {
		Notes& notes = m_song.getVocalTrack(name).notes;
		auto it = notes.rbegin();
		if (!notes.empty() && it->type != Note::SLEEP && it->begin == it->end) notes.pop_back();
		if (notes.empty()) m_song.eraseVocalTrack(name);
	}

}

bool SongParser::txtParseField(std::string const& line) {
	if (line.empty()) return true;
	if (line[0] != '#') return false;
	std::string::size_type pos = line.find(':');
	if (pos == std::string::npos) throw std::runtime_error("Invalid txt format, should be #key:value");
	std::string key = boost::trim_copy(line.substr(1, pos - 1));
	std::string value = boost::trim_copy(line.substr(pos + 1));
	if (value.empty()) return true;

	// Parse header data that is stored in SongParser rather than in song (and thus needs to be read every time)
	if (key == "BPM") assign(m_bpm, value);
	else if (key == "RELATIVE") assign(m_relative, value);
	else if (key == "GAP") { assign(m_gap, value); m_gap *= 1e-3; }
	else if (key == "DUETSINGERP1" || key == "P1") m_song.insertVocalTrack(TrackName::LEAD_VOCAL, VocalTrack(value.substr(value.find_first_not_of(" "))));
	// Strong hint that this is a duet, so it will be readily displayed with two singers in browser and properly filtered
	else if (key == "DUETSINGERP2" || key == "P2") m_song.insertVocalTrack(DUET_P2, VocalTrack(value.substr(value.find_first_not_of(" "))));

	if (m_song.loadStatus >= Song::HEADER) return true;  // Only re-parsing now, skip any other data

	// Parse header data that is directly stored in m_song
	if (key == "TITLE") m_song.title = value.substr(value.find_first_not_of(" :"));
	else if (key == "ARTIST") m_song.artist = value.substr(value.find_first_not_of(" "));
	else if (key == "EDITION") m_song.edition = value.substr(value.find_first_not_of(" "));
	else if (key == "GENRE") m_song.genre = value.substr(value.find_first_not_of(" "));
	else if (key == "CREATOR") m_song.creator = value.substr(value.find_first_not_of(" "));
	else if (key == "COVER") m_song.cover = fs::absolute(value, m_song.path);
	else if (key == "MP3") m_song.music["background"] = fs::absolute(value, m_song.path);
	else if (key == "VOCALS") m_song.music["vocals"] = fs::absolute(value, m_song.path);
	else if (key == "VIDEO") m_song.video = fs::absolute(value, m_song.path);
	else if (key == "BACKGROUND") m_song.background = fs::absolute(value, m_song.path);
	else if (key == "START") assign(m_song.start, value);
	else if (key == "VIDEOGAP") assign(m_song.videoGap, value);
	else if (key == "PREVIEWSTART") assign(m_song.preview_start, value);
	else if (key == "LANGUAGE") m_song.language = value.substr(value.find_first_not_of(" "));
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
	if (line[0] == 'P') {
		if (m_relative) // FIXME?
			throw std::runtime_error("Relative note timing not supported with multiple singers");
		if (line.size() < 2) throw std::runtime_error("Invalid player info line [too short]: " + line);
		else if (line[1] == '1') m_curSinger = P1;
		else if (line[1] == '2') m_curSinger = P2;
		else if (line[1] == '3') m_curSinger = BOTH;
		else if (line.size() < 3) throw std::runtime_error("Invalid player info line [too short]: " + line);
		else if (line[2] == '1') m_curSinger = P1;
		else if (line[2] == '2') m_curSinger = P2;
		else if (line[2] == '3') m_curSinger = BOTH;
		else throw std::runtime_error("Invalid player info line [malformed]: " + line);
		resetNoteParsingState();
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
	VocalTrack& vocal = (m_curSinger & P1)
	  ? m_song.getVocalTrack(TrackName::LEAD_VOCAL)
	  : m_song.getVocalTrack(DUET_P2);
	Notes& notes = vocal.notes;
	if (m_relative && notes.empty()) m_relativeShift = ts;
	m_prevts = ts;
	// FIXME: These work-arounds don't work for P3 (both singers) case
	if (n.begin < m_prevtime) {
		// Oh no, overlapping notes (b0rked file)
		// Can't do this because too many songs are b0rked: throw std::runtime_error("Note overlaps with previous note");
		if (notes.size() >= 1) {
			Note& p = notes.back();
			// Workaround for songs that use semi-random timestamps for sleep
			if (p.type == Note::SLEEP) {
				p.end = p.begin;
				auto it = notes.rbegin();
				Note& p2 = *++it;
				if (p2.end < n.begin) p.begin = p.end = n.begin;
			}
			// Can we just make the previous note shorter?
			if (p.begin <= n.begin) p.end = n.begin;
			else if (m_song.b0rked.empty()) { // Cannot fix, warn and skip
				std::string msg = "Skipped overlapping notes.\n";
				m_song.b0rked = msg;
				std::clog << "songparser/notice: " + m_song.filename.string() + ": " + msg << std::endl;
				return true;
			}
		} else throw std::runtime_error("The first note has negative timestamp");
	}
	double prevtime = m_prevtime;
	m_prevtime = n.end;
	if (n.type != Note::SLEEP && n.end > n.begin) {
		vocal.noteMin = std::min(vocal.noteMin, n.note);
		vocal.noteMax = std::max(vocal.noteMax, n.note);
	}
	if (n.type == Note::SLEEP) {
		if (notes.empty()) return true; // Ignore sleeps at song beginning
		n.begin = n.end = prevtime; // Normalize sleep notes
	}
	notes.push_back(n);
	if (m_curSinger == BOTH)
		m_song.getVocalTrack(DUET_P2).notes.push_back(n);
	return true;
}

