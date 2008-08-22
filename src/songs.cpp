#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <glob.h>
#include <songs.h>
#include <screen.h>
#include <xtime.h>
// math.h needed for C99 stuff
#include <math.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

double coverMathAdvanced::getPosition(){
	const double acceleration = 50.0; // the coefficient of velocity changes (animation speed)
	const double overshoot = 0.95; // Over 1.0 decelerates too late, less than 1.0 decelerates too early
	if (m_songs == 0) return m_target;
	boost::xtime curtime = now();
	double duration = seconds(curtime) - seconds(m_time);
	m_time = curtime;
	if (!(duration > 0.0)) return m_position; // Negative value or NaN, or no songs - skip processing
	if (duration > 1.0) duration = 1.0; // No more than one second per frame
	std::size_t rounds = 1.0 + 1000.0 * duration; // 1 ms or shorter timesteps
	double t = duration / rounds;
	for (std::size_t i = 0; i < rounds; ++i) {
		double d = remainder(m_target - m_position, m_songs); // Distance (via shortest way)
		// Allow it to stop nicely, without jitter
		if (std::abs(m_velocity) < 0.1 && std::abs(d) < 0.001) {
			m_velocity = 0.0;
			m_position = m_target;
			break;
		}
		double a = d > 0.0 ? acceleration : -acceleration; // Acceleration vector
		// Are we going to right direction && can we stop in time if we start decelerating now?
		if (d * m_velocity > 0.0 && std::abs(m_velocity) > 2.0 * overshoot * acceleration * d / m_velocity) a *= -1.0;
		// Apply Newtonian mechanics
		m_velocity += t * a;
		m_position += t * m_velocity;
	}
	return m_position = remainder(m_position, m_songs); // Return & store normalized position
}


std::string MusicalScale::getNoteStr(double freq) const {
	int id = getNoteId(freq);
	if (id == -1) return std::string();
	static const char * note[12] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
	std::ostringstream oss;
	// Acoustical Society of America Octave Designation System
	//int octave = 2 + id / 12;
	oss << note[id%12] << " " << int(round(freq)) << " Hz";
	return oss.str();
}

unsigned int MusicalScale::getNoteNum(int id) const {
	// C major scale
	int n = id % 12;
	return (n + (n > 4)) / 2;
}

bool MusicalScale::isSharp(int id) const {
	if (id < 0) throw std::logic_error("MusicalScale::isSharp: Invalid note ID");
	// C major scale
	switch (id % 12) {
	  case 1: case 3: case 6: case 8: case 10: return true;
	}
	return false;
}

double MusicalScale::getNoteFreq(int id) const {
	if (id == -1) return 0.0;
	return m_baseFreq * pow(2.0, (id - m_baseId) / 12.0);
}

int MusicalScale::getNoteId(double freq) const {
	double note = getNote(freq);
	if (note >= 0.0 && note < 100.0) return int(note + 0.5);
	return -1;
}

double MusicalScale::getNote(double freq) const {
	if (freq < 1.0) return std::numeric_limits<double>::quiet_NaN();
	return m_baseId + 12.0 * log(freq / m_baseFreq) / log(2);
}

double MusicalScale::getNoteOffset(double freq) const {
	double frac = freq / getNoteFreq(getNoteId(freq));
	return 12.0 * log(frac) / log(2);
}

double Note::diff(double n) const {	return remainder(n - note, 12.0); }
double Note::maxScore() const { return scoreMultiplier(0.0) * (end - begin); }

double Note::score(double n, double b, double e) const {
	double len = std::min(e, end) - std::max(b, begin);
	if (len <= 0.0 || !(n > 0.0)) return 0.0;
	return scoreMultiplier(std::abs(diff(n))) * len;
}

double Note::scoreMultiplier(double error) const {
	double max = 0.0;
	switch (type) {
	  case FREESTYLE: return 1.0;
	  case NORMAL: max = 1.0; break;
	  case GOLDEN: max = 2.0; break;
	  case SLEEP: break;
	}
	return std::min(1.0, std::max(0.0, 1.5 - error)) * max;
}


class SongParser {
  public:
	struct Exception: public std::runtime_error {
		Exception(std::string const& msg, unsigned int linenum):
		  runtime_error(msg), m_linenum(linenum) {}
		unsigned int line() const { return m_linenum; }
	  private:
		unsigned int m_linenum;
	};
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
		if (!m_f.is_open()) throw Exception("Could not open TXT file", 0);
		std::string line;
		try {
			while (getline(line) && parseField(line)) {};
			if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
			if (m_bpm != 0.0) addBPM(0, m_bpm);
			while (parseNote(line) && getline(line)) {};
		} catch (std::runtime_error& e) {
			throw Exception(e.what(), m_linenum);
		}
		if (s.notes.empty()) throw Exception("No notes", m_linenum);
		// Workaround for the terminating : 1 0 0 line, written by some converters
		if (s.notes.back().type != Note::SLEEP && s.notes.back().begin == s.notes.back().end) s.notes.pop_back();
		// Adjust negative notes
		if (m_song.noteMin <= 0) {
			unsigned int shift = (1 - m_song.noteMin / 12) * 12;
			m_song.noteMin += shift;
			m_song.noteMax += shift;
			for (Song::notes_t::iterator it = s.notes.begin(); it != s.notes.end(); ++it) it->note += shift;
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
		Song::notes_t& notes = m_song.notes;
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
					Song::notes_t::reverse_iterator it = notes.rbegin();
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

Song::Song(std::string const& _path, std::string const& _filename):
  noteMin(std::numeric_limits<int>::max()),
  noteMax(std::numeric_limits<int>::min()),
  path(_path),
  filename(_filename),
  videoGap(),
  start(),
  m_scoreFactor(),
  m_score(),
  m_scoreTime()
{
	SongParser(*this);
}

void Song::reload() {
	notes.clear();
	category.clear();
	genre.clear();
	edition.clear();
	title.clear();
	artist.clear();
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
	try {
		SongParser(*this);
	} catch (...) {}
	reset();
}

void Song::reset() {
	m_score = 0.0;
	m_scoreTime = 0.0;
	m_scoreIt = notes.begin();
}

void Song::resetPitchGraph() {
	timePitchGraph.clear();
	pitchPitchGraph.clear();	
	volumePitchGraph.clear();	
}

void Song::updatePitchGraph(double time, double pitch, double volume, bool draw) {
	timePitchGraph.push_back(time);
	pitchPitchGraph.push_back(pitch);
	volumePitchGraph.push_back(volume);
	drawPitchGraph.push_back(draw);
}

void Song::update(double time, double freq) {
	if (time <= m_scoreTime) return;
	while (m_scoreIt != notes.end()) {
		if (freq > 0.0) m_score += m_scoreFactor * m_scoreIt->score(scale.getNote(freq), m_scoreTime, time);
		if (time < m_scoreIt->end) break;
		++m_scoreIt;
	}
	m_scoreTime = time;
	m_score = std::min(1.0, std::max(0.0, m_score));
}

bool operator<(Song const& l, Song const& r) {
	if (l.artist != r.artist) return l.artist < r.artist;
	if (l.title != r.title) return l.title < r.title;
	return l.filename < r.filename;
	// If filenames are identical, too, the songs are considered the same.
}

Songs::Songs(std::set<std::string> const& songdirs): m_songdirs(songdirs), math_cover(), m_order() {
	reload();
}

void Songs::reload() {
	songlist_t songs;
	for (std::set<std::string>::const_iterator it = m_songdirs.begin(); it != m_songdirs.end(); ++it) {
		glob_t _glob;
		std::string pattern = *it + "*/*.[tT][xX][tT]";
		std::cout << ">>> Scanning " << *it << ": " << std::flush;
		glob (pattern.c_str(), GLOB_NOSORT, NULL, &_glob);
		std::cout << _glob.gl_pathc << " song files found." << std::endl;
		for (unsigned int i = 0 ; i < _glob.gl_pathc ; i++) {
			char* txtfilename = strrchr(_glob.gl_pathv[i],'/'); txtfilename[0] = '\0'; txtfilename++;
			std::string path = _glob.gl_pathv[i];
			std::string::size_type pos = path.rfind('/');
			if (pos < path.size() - 1) pos += 1; else pos = 0;
			std::cout << "\r  " << std::setiosflags(std::ios::left) << std::setw(70) << path.substr(pos, 70) << "\x1B[K" << std::flush;
			try {
				songs.insert(new Song(path + "/", txtfilename));
			} catch (SongParser::Exception& e) {
				std::cout << "FAIL\n    " << txtfilename;
				if (e.line()) std::cout << " line " << e.line();
				std::cout << ": " << e.what() << std::endl;
			}
		}
		std::cout << "\r\x1B[K" << std::flush;
		globfree(&_glob);
	}
	m_songs.swap(songs);
	filter_internal();
}

class Songs::RestoreSel {
	Songs& m_s;
	Song* m_sel;
  public:
	RestoreSel(Songs& s): m_s(s), m_sel(s.empty() ? NULL : &s.current()) {}
	~RestoreSel() {
		int pos = 0;
		if (m_sel) {
			filtered_t& f = m_s.m_filtered;
			filtered_t::iterator it = std::find(f.begin(), f.end(), m_sel);
			m_s.math_cover.setTarget(0, 0);
			if (it != f.end()) pos = it - f.begin();
		}
		m_s.math_cover.setTarget(pos, m_s.size());
	}
};

Song& Songs::near(double pos) {
	std::size_t s = m_filtered.size();
	pos = round(remainder(pos, s) + s);
	return (*this)[std::size_t(pos) % s];
}

void Songs::randomize() {
	RestoreSel restore(*this);
	m_order = 0;
	sort_internal();
}

void Songs::setFilter(std::string const& val) {
	if (m_filter == val) return;
	m_filter = val;
	filter_internal();
}

void Songs::filter_internal() {
	RestoreSel restore(*this);
	filtered_t filtered;
	try {
		for (songlist_t::iterator it = m_songs.begin(); it != m_songs.end(); ++it) {
			if (regex_search(it->str(), boost::regex(m_filter ,boost::regex_constants::icase))) filtered.push_back(&*it);
		}
	} catch (...) {
		filtered.clear();
		for (songlist_t::iterator it = m_songs.begin(); it != m_songs.end(); ++it) {
			filtered.push_back(&*it);
		}
	}
	m_filtered.swap(filtered);
	math_cover.setTarget(0, 0);
	sort_internal();
}

class CmpByField {
	std::string Song::* m_field;
  public:
	CmpByField(std::string Song::* field): m_field(field) {}
	bool operator()(Song const& left , Song const& right) {
		if (left.*m_field == right.*m_field) return left < right;
		return left.*m_field < right.*m_field;
	}
	bool operator()(Song const* left , Song const* right) {
		return operator()(*left, *right);
	}
};

namespace {
	std::string pathtrim(std::string path) {
		std::string::size_type pos = path.rfind('/', path.size() - 1);
		pos = path.rfind('/', pos - 1);
		pos = path.rfind('/', pos - 1);
		if (pos == std::string::npos) pos = 0; else ++pos;
		return path.substr(pos, path.size() - pos - 1);
	}
}

static char const* order[] = {
	"random order",
	"by song",
	"by artist",
	"by edition",
	"by genre",
	"by path"
};

static const int orders = sizeof(order) / sizeof(*order);

std::string Songs::sortDesc() const {
	std::string str = order[m_order];
	if (!empty()) {
		if (m_order == 3) str += " (" + current().edition + ")";
		if (m_order == 4) str += " (" + current().genre + ")";
		if (m_order == 5) str += " (" + pathtrim(current().path) + ")";
	}
	return str;
}

void Songs::sortChange(int diff) {
	m_order = (m_order + diff) % orders;
	if (m_order < 0) m_order += orders;
	RestoreSel restore(*this);
	sort_internal();
}

void Songs::sort_internal() {
	switch (m_order) {
	  case 0: std::random_shuffle(m_filtered.begin(), m_filtered.end()); break;
	  case 1: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::title)); break;
	  case 2: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::artist)); break;
	  case 3: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::edition)); break;
	  case 4: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::genre)); break;
	  case 5: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::path)); break;
	  default: throw std::logic_error("Internal error: unknown sort order in Songs::sortChange");
	}
}

void Songs::dump(std::ostream& os, std::string const& sort) {
	os << m_filtered.size() << " songs, ";
	if (sort == "title") m_order = 1;
	else if (sort == "artist") m_order = 2;
	else if (sort == "path") m_order = 5;
	else { os << "invalid sort order specified" << std::endl; return; }
	os << order[m_order] << '\n' << std::string(40, '-') << '\n';
	RestoreSel restore(*this);
	sort_internal();
	for (filtered_t::const_iterator it = m_filtered.begin(); it != m_filtered.end(); ++it) {
		os << "  " << (*it)->str() << std::endl;
	}
}

