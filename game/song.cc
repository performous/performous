#include "song.hh"

#include <boost/lexical_cast.hpp>
#include <glibmm/ustring.h>
#include <algorithm>
#include <fstream>

class SongParser {
  public:
	SongParser(Song& s):
	  m_song(s),
	  m_f((s.path + s.filename).c_str()),
	  m_linenum(),
	  m_relative(),
	  m_gap(),
	  m_bpm(),
	  m_prevts(),
	  m_relativeShift(),
	  m_maxScore()
	{
		if (!m_f.is_open()) throw SongParserException("Could not open TXT file", 0);
		std::string line;
		try {
			while (getline(line) && parseField(line)) {};
			if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
			if (m_bpm != 0.0) addBPM(0, m_bpm);
			while (parseNote(line) && getline(line)) {};
		} catch (std::runtime_error& e) {
			throw SongParserException(e.what(), m_linenum);
		}
		if (s.notes.empty()) throw SongParserException("No notes", m_linenum);
		// Workaround for the terminating : 1 0 0 line, written by some converters
		if (s.notes.back().type != Note::SLEEP && s.notes.back().begin == s.notes.back().end) s.notes.pop_back();
		// Adjust negative notes
		if (m_song.noteMin <= 0) {
			unsigned int shift = (1 - m_song.noteMin / 12) * 12;
			m_song.noteMin += shift;
			m_song.noteMax += shift;
			for (Notes::iterator it = s.notes.begin(); it != s.notes.end(); ++it) it->note += shift;
		}
		m_song.m_scoreFactor = 1.0 / m_maxScore;
	}
  private:
	Song& m_song;
	std::ifstream m_f;
	unsigned int m_linenum;
	bool getline(std::string& line) { ++m_linenum; return std::getline(m_f, line); }
	bool m_relative;
	double m_gap;
	double m_bpm;
	void assign(int& var, std::string const& str) {
		var = boost::lexical_cast<int>(str);
	}
	void assign(double& var, std::string str) {
		std::replace(str.begin(), str.end(), ',', '.'); // Fix decimal separators
		var = boost::lexical_cast<double>(str);
	}
	void assign(bool& var, std::string const& str) {
		if (str == "YES" || str == "yes" || str == "1") var = true;
		else if (str == "NO" || str == "no" || str == "0") var = false;
		else throw std::logic_error("Invalid boolean value: " + str);
	}
	bool parseField(std::string const& line) {
		if (line.empty()) return true;
		if (line[0] != '#') return false;
		std::string::size_type pos = line.find(':');
		if (pos == std::string::npos) throw std::runtime_error("Invalid format, should be #key=value");
		std::string key = line.substr(1, pos - 1);
		std::string::size_type pos2 = line.find_last_not_of(" \t\r");
		std::string value = line.substr(pos + 1, pos2 - pos);
		if (value.empty()) throw std::runtime_error("Value missing from key " + key);
		if (key == "TITLE") m_song.title = value.substr(value.find_first_not_of(" :"));
		else if (key == "ARTIST") m_song.artist = value.substr(value.find_first_not_of(" "));
		else if (key == "EDITION") m_song.edition = value.substr(value.find_first_not_of(" "));
		else if (key == "GENRE") m_song.genre = value.substr(value.find_first_not_of(" "));
		else if (key == "CREATOR") m_song.creator = value.substr(value.find_first_not_of(" "));
		else if (key == "COVER") m_song.cover = value;
		else if (key == "MP3") m_song.mp3 = value;
		else if (key == "VIDEO") m_song.video = value;
		else if (key == "BACKGROUND") m_song.background = value;
		else if (key == "START") assign(m_song.start, value);
		else if (key == "VIDEOGAP") assign(m_song.videoGap, value);
		else if (key == "RELATIVE") assign(m_relative, value);
		else if (key == "GAP") { assign(m_gap, value); m_gap *= 1e-3; }
		else if (key == "BPM") assign(m_bpm, value);
		return true;
	}
	unsigned int m_prevts;
	unsigned int m_relativeShift;
	double m_maxScore;
	struct BPM {
		BPM(double _begin, unsigned int _ts, double bpm): begin(_begin), step(0.25 * 60.0 / bpm), ts(_ts) {}
		double begin; // Time in seconds
		double step; // Seconds per quarter note
		unsigned int ts;
	};
	typedef std::vector<BPM> bpms_t;
	bpms_t m_bpms;
	void addBPM(unsigned int ts, double bpm) {
		if (!m_bpms.empty() && m_bpms.back().ts >= ts) throw std::runtime_error("Invalid BPM timestamp");
		if (!(bpm >= 1.0 && bpm < 1e12)) throw std::runtime_error("Invalid BPM value");
		m_bpms.push_back(BPM(tsTime(ts), ts, bpm));
	}
	bool parseNote(std::string line) {
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
		if (notes.empty() && n.type == Note::SLEEP) throw std::runtime_error("Song cannot begin with sleep");
		m_prevts = ts;
		double prevtime = notes.empty() ? 0.0 : notes.back().end;
		if (n.begin < prevtime) {
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
		if (n.type != Note::SLEEP && n.end > n.begin) {
			m_song.noteMin = std::min(m_song.noteMin, n.note);
			m_song.noteMax = std::max(m_song.noteMax, n.note);
			m_maxScore += n.maxScore();
		}
		notes.push_back(n);
		return true;
	}
	double tsTime(unsigned int ts) const {
		if (m_bpms.empty()) {
			if (ts != 0) throw std::runtime_error("BPM data missing");
			return m_gap;
		}
		for (std::vector<BPM>::const_reverse_iterator it = m_bpms.rbegin(); it != m_bpms.rend(); ++it) {
			if (it->ts <= ts) return it->begin + (ts - it->ts) * it->step;
		}
		throw std::logic_error("INTERNAL ERROR: BPM data invalid");
	}
};

void Song::reload(bool errorIgnore) {
	notes.clear();
	category.clear();
	genre.clear();
	edition.clear();
	title.clear();
	artist.clear();
	collateByTitle.clear();
	collateByArtist.clear();
	text.clear();
	creator.clear();
	mp3.clear();
	cover.clear();
	background.clear();
	video.clear();
	noteMin = std::numeric_limits<int>::max();
	noteMax = std::numeric_limits<int>::min();
	videoGap = 0.0;
	start = 0.0;
	m_scoreFactor = 0.0;
	try { SongParser(*this); } catch (...) { if (!errorIgnore) throw; }
	collateUpdate();
}
#include <iostream>
void Song::collateUpdate() {
	collateByTitle = collate(title + artist) + '\0' + filename;
	collateByArtist = collate(artist + title) + '\0' + filename;
}

std::string Song::collate(std::string const& str) {
	Glib::ustring ustr = str, ustr2;
	// Remove all non-alnum characters
	for (Glib::ustring::iterator it = ustr.begin(), end = ustr.end(); it != end; ++it) {
		if (Glib::Unicode::isalnum(*it)) ustr2 += Glib::Unicode::tolower(*it);
	}
	return ustr2;
	// Should use ustr2.casefold_collate_key() instead of tolower, but it seems to be crashing...
}

