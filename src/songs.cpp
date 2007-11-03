#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <glob.h>
#include <songs.h>
#include <screen.h>
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

std::string MusicalScale::getNoteStr(double freq) const {
	int id = getNoteId(freq);
	if (id == -1) return std::string();
	static const char * note[12] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
	std::ostringstream oss;
	// Acoustical Society of America Octave Designation System
	//int octave = 2 + id / 12;
	oss << note[id%12] << " " << (int)round(freq) << " Hz";
	return oss.str();
}

unsigned int MusicalScale::getNoteNum(int id) const {
	switch (id % 12) {
	  case 0: return 0;
	  case 1: return 0;
	  case 2: return 1;
	  case 3: return 1;
	  case 4: return 2;
	  case 5: return 3;
	  case 6: return 3;
	  case 7: return 4;
	  case 8: return 4;
	  case 9: return 5;
	  case 10: return 5;
	  default: return 6;
	}
}

bool MusicalScale::isSharp(int id) const {
	if (id < 0) throw std::logic_error("MusicalScale::isSharp: Invalid note ID");
	id %= 12;
	switch (id) {
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

double Note::diff(double n) const {
	return remainder(n - note, 12.0);
	/*
	int d = (6 + n - note) % 12;
	if (d < 0) d += 12;
	return d - 6;
	*/
}

double Note::maxScore() const {
	return scoreMultiplier(note) * (end - begin);
}

double Note::score(double n, double b, double e) const {
	double len = std::min(e, end) - std::max(b, begin);
	if (len <= 0.0) return 0.0;
	return scoreMultiplier(n) * len;
}

double Note::scoreMultiplier(double n) const {
	double max = 0.0;
	switch (type) {
	  case FREESTYLE: return 1.0;
	  case NORMAL: max = 1.0; break;
	  case GOLDEN: max = 2.0; break;
	  case SLEEP: break;
	}
	if (!(n > 0.0)) return 0.0;
	double error = std::abs(diff(n));
	if (error < 0.5) return max;
	return std::max(0.0, 1.5 - error) * max;
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
			while (getline(line) && parseField(line));
			if (s.title.empty() || s.artist.empty()) throw std::runtime_error("Required header fields missing");
			if (m_bpm != 0.0) addBPM(0, m_bpm);
			while (parseNote(line) && getline(line));
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
					Note& p2 = notes[notes.size() - 2];
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
  m_coverSurf(),
  m_backgroundSurf(),
  m_scoreFactor(),
  m_score(),
  m_scoreTime()
{
	SongParser(*this);
}

void Song::reset() {
	m_score = 0.0;
	m_scoreTime = 0.0;
	m_scoreIt = notes.begin();
	std::cout << "reset" << std::endl;
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

void Song::loadCover() {
	if (m_coverSurf || cover.empty()) return;
	double width = CScreenManager::getSingletonPtr()->getWidth();
	double height = CScreenManager::getSingletonPtr()->getHeight();
	std::string file = path + cover;
	std::string::size_type extpos = file.rfind('.');
	std::string ext = (extpos == std::string::npos) ? "" : file.substr(extpos);
	SDL_RWops* rwop = SDL_RWFromFile(file.c_str(), "rb");
	SDL_Surface* surf = NULL;
	if (ext == ".jpg" || ext == ".JPG" || ext == ".jpeg") surf = IMG_LoadJPG_RW(rwop);
	else if (ext == ".png" || ext == ".PNG") surf = IMG_LoadPNG_RW(rwop);
	else std::cout << "Cover image file " << file << " has unknown extension" << std::endl;
	if (rwop) SDL_RWclose(rwop);
	if (surf == NULL) m_coverSurf = NULL;
	else {
		// Here we want to have cover of 256x256 in 800x600 and scale it if the resolution is different
		double w = width * 256.0 / 800.0;
		double h = height * 256.0 / 600.0;
		m_coverSurf = zoomSurface(surf, w / surf->w, h / surf->h, 1);
		SDL_FreeSurface(surf);
	}
	// Prevent trying to reload the same cover
	if (!m_coverSurf) cover.clear();
}

void Song::loadBackground() {
	if (m_backgroundSurf || background.empty()) return;
	double width = CScreenManager::getSingletonPtr()->getWidth();
	double height = CScreenManager::getSingletonPtr()->getHeight();
	std::string file = path + background;
	std::string::size_type extpos = file.rfind('.');
	std::string ext = (extpos == std::string::npos) ? "" : file.substr(extpos);
	SDL_RWops* rwop = SDL_RWFromFile(file.c_str(), "rb");
	SDL_Surface* surf = NULL;
	if (ext == ".jpg" || ext == ".JPG" || ext == ".jpeg") surf = IMG_LoadJPG_RW(rwop);
	else if (ext == ".png" || ext == ".PNG") surf = IMG_LoadPNG_RW(rwop);
	else std::cout << "Background image file " << file << " has unknown extension" << std::endl;
	if (rwop) SDL_RWclose(rwop);
	if (!surf) m_backgroundSurf = NULL;
	else {
		m_backgroundSurf = zoomSurface(surf, width / surf->w, height / surf->h, 1);
		SDL_FreeSurface(surf);
	}
	// Prevent trying to reload the same cover
	if (!m_backgroundSurf) background.clear();
}

void Song::unloadCover() {
	if (m_coverSurf) SDL_FreeSurface(m_coverSurf);
	m_coverSurf = NULL;
}

void Song::unloadBackground() {
	if (m_backgroundSurf) SDL_FreeSurface(m_backgroundSurf);
	m_backgroundSurf = NULL;
}

bool operator<(Song const& l, Song const& r) {
	if (l.artist != r.artist) return l.artist < r.artist;
	if (l.title != r.title) return l.title < r.title;
	return l.filename < r.filename;
	// If filenames are identical, too, the songs are considered the same.
}

Songs::Songs(std::set<std::string> const& songdirs): m_songdirs(songdirs), m_current(), m_order() {
	std::string file = CScreenManager::getSingletonPtr()->getThemePathFile("no_cover.png");
	SDL_RWops* rwop_nocover = SDL_RWFromFile(file.c_str(), "rb");
	SDL_Surface* tmp = IMG_LoadPNG_RW(rwop_nocover);
	if (!tmp) throw std::runtime_error("Could not load " + file);
	double w = CScreenManager::getSingletonPtr()->getWidth() * 256.0 / 800.0;
	double h = CScreenManager::getSingletonPtr()->getHeight() * 256.0 / 600.0;
	surface_nocover = zoomSurface(tmp, w / tmp->w, h / tmp->h, 1);
	SDL_FreeSurface(tmp);
	if (rwop_nocover) SDL_RWclose(rwop_nocover);
	if (surface_nocover == NULL) throw std::runtime_error("Cannot load " + file);
	reload();
}

Songs::~Songs() {
	SDL_FreeSurface(surface_nocover);
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
	setFilter("");
}

class Songs::RestoreSel {
	Songs& m_s;
	Song* m_sel;
  public:
	RestoreSel(Songs& s): m_s(s), m_sel(s.empty() ? NULL : &s.current()) {}
	~RestoreSel() {
		m_s.random();
		if (!m_sel) return;
		filtered_t& f = m_s.m_filtered;
		filtered_t::iterator it = std::find(f.begin(), f.end(), m_sel);
		if (it != f.end()) m_s.m_current = it - f.begin();
	}
};

void Songs::random() {
	m_current = empty() ? 0 : std::rand() % m_filtered.size();
}

void Songs::setFilter(std::string const& val) {
	RestoreSel restore(*this);
	filtered_t filtered;
	try {
		for (songlist_t::iterator it = m_songs.begin(); it != m_songs.end(); ++it) {
			if (regex_search(it->str(), boost::regex(val))) filtered.push_back(&*it);
		}
	} catch (...) {
		filtered.clear();
		for (songlist_t::iterator it = m_songs.begin(); it != m_songs.end(); ++it) {
			filtered.push_back(&*it);
		}
	}
	m_filtered.swap(filtered);
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

static char const* order[] = {
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
		if (m_order == 2) str += " (" + current().edition + ")";
		if (m_order == 3) str += " (" + current().genre + ")";
		if (m_order == 4) str += " (" + current().path + ")";
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
	  case 0: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::title)); break;
	  case 1: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::artist)); break;
	  case 2: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::edition)); break;
	  case 3: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::genre)); break;
	  case 4: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&Song::path)); break;
	  default: throw std::logic_error("Internal error: unknown sort order in Songs::sortChange");
	}
}

