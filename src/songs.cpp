#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <glob.h>
#include <songs.h>
#include <screen.h>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>

Note::Note(std::string const& line, int* relShift) {
	std::istringstream iss(line);
	type = Type(iss.get());
	switch (type) {
	  case NORMAL:
	  case FREESTYLE:
	  case GOLDEN:
		if (!(iss >> timestamp >> length >> note)) throw std::runtime_error("Invalid note line format");
		if (iss.get() == ' ') std::getline(iss, syllable);
		if (relShift) timestamp += *relShift;
		break;
	  case SLEEP:
		if (iss >> timestamp) length = (iss >> length) ? length - timestamp : 0;
		if (relShift) *relShift = (timestamp += *relShift) + length;
		break;
	  default: throw std::runtime_error("Unknown note type");
	}
}

CSong::CSong(std::string const& _path, std::string const& _filename):
  path(_path),
  filename(_filename),
  genre(),
  edition(),
  title(),
  artist(),
  text(),
  creator(),
  cover(),
  coverSurf(),
  mp3(),
  background(),
  backgroundSurf(),
  video(),
  videoGap(),
  relative(),
  gap(),
  noteMin(std::numeric_limits<int>::max()),
  noteMax(std::numeric_limits<int>::min())
{
	std::ifstream f((path + filename).c_str());
	if (!f.is_open()) throw std::runtime_error(filename + ": Cannot open file");
	std::string line;
	std::size_t linenum = 0;
	try {
		while (++linenum, std::getline(f, line) && parseField(line));
		if (title.empty() || artist.empty()) throw std::runtime_error("Required header fields missing");
		int relativeShift = 0;
		int prevtime = 0;
		do {
			if (line.empty() || line == "\r") continue;
			if (line[0] == '#') throw std::runtime_error("Key found in the middle of notes");
			if (line[line.size() - 1] == '\r') line.erase(line.size() - 1);
			if (line[0] == 'E') break;
			notes.push_back(Note(line, relative ? &relativeShift : NULL));
			Note& n = notes.back();
			if (notes.size() == 1 && n.type == Note::SLEEP) throw std::runtime_error("Song cannot begin with sleep");
			if (n.timestamp < prevtime) {
				// Oh no, overlapping notes (b0rked file)
				// Can't do this because too many songs are b0rked: throw std::runtime_error("Note overlaps with previous note");
				if (notes.size() >= 2) {
					Note& p = notes[notes.size() - 2];
					// Workaround for songs that use semi-random timestamps for sleep
					if (p.type == Note::SLEEP) {
						p.length = 0;
						Note& p2 = notes[notes.size() - 3];
						if (p2.timestamp + p2.length < n.timestamp) p.timestamp = n.timestamp;
					}
					// Can we just make the previous note shorter?
					if (p.timestamp <= n.timestamp) p.length = n.timestamp - p.timestamp;
					else throw std::runtime_error("Note overlaps with earlier notes");
				} else throw std::runtime_error("The first note has negative timestamp");
			}
			prevtime = n.timestamp + n.length;
			if (n.type != Note::SLEEP && n.length > 0) {
				noteMin = std::min(noteMin, n.note);
				noteMax = std::max(noteMax, n.note);
			}
		} while (++linenum, std::getline(f, line));
		if (notes.empty()) throw std::runtime_error("No notes");
		// Workaround for terminating : 1 0 0 line, written by some converters
		if (notes.back().type != Note::SLEEP && notes.back().length == 0) notes.pop_back();
		// Adjust negative notes
		if (noteMin <= 0) {
			unsigned int shift = (1 - noteMin / 12) * 12;
			noteMin += shift;
			noteMax += shift;
			for(unsigned i = 0; i < notes.size(); ++i) notes[i].note += shift;
		}
	} catch (std::exception& e) {
		std::ostringstream oss;
		oss << filename << " line " << linenum << ": " << e.what();
		throw std::runtime_error(oss.str());
	}
}

namespace {
	void assign(int& var, std::string const& str) {
		var = boost::lexical_cast<int>(str);
	}
	void assign(float& var, std::string str) {
		std::replace(str.begin(), str.end(), ',', '.'); // Fix decimal separators
		var = boost::lexical_cast<float>(str);
	}
	void assign(bool& var, std::string const& str) {
		if (str == "YES" || str == "yes" || str == "1") var = true;
		else if (str == "NO" || str == "no" || str == "0") var = false;
		else throw std::logic_error("Invalid boolean value: " + str);
	}
}

bool CSong::parseField(std::string const& line) {
	if (line.empty()) return true;
	if (line[0] != '#') return false;
	std::string::size_type pos = line.find(':');
	if (pos == std::string::npos) throw std::runtime_error("Invalid format, should be #key=value");
	std::string key = line.substr(1, pos - 1);
	std::string::size_type pos2 = line.find_last_not_of(" \t\r");
	std::string value = line.substr(pos + 1, pos2 - pos);
	if (value.empty()) throw std::runtime_error("Value missing from key " + key);
	if (key == "TITLE") title = value.substr(value.find_first_not_of(" :"));
	else if (key == "ARTIST") artist = value.substr(value.find_first_not_of(" "));
	else if (key == "EDITION") edition = value.substr(value.find_first_not_of(" "));
	else if (key == "GENRE") genre = value.substr(value.find_first_not_of(" "));
	else if (key == "CREATOR") creator = value.substr(value.find_first_not_of(" "));
	else if (key == "COVER") cover = value;
	else if (key == "MP3") mp3 = value;
	else if (key == "VIDEO") video = value;
	else if (key == "BACKGROUND") background = value;
	else if (key == "START") assign(start, value);
	else if (key == "VIDEOGAP") assign(videoGap, value);
	else if (key == "RELATIVE") assign(relative, value);
	else if (key == "GAP") assign(gap, value);
	else if (key == "BPM") {
		TBpm tmp;
		tmp.start = 0.0;
		assign(tmp.bpm, value);
		bpm.push_back(tmp);
	}
	return true;
}

void CSong::loadCover() {
	if (coverSurf || cover.empty()) return;
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
	if (surf == NULL) coverSurf = NULL;
	else {
		// Here we want to have cover of 256x256 in 800x600 and scale it if the resolution is different
		double w = width * 256.0 / 800.0;
		double h = height * 256.0 / 600.0;
		coverSurf = zoomSurface(surf, w / surf->w, h / surf->h, 1);
		SDL_FreeSurface(surf);
	}
	// Prevent trying to reload the same cover
	if (!coverSurf) cover.clear();
}

void CSong::loadBackground() {
	if (backgroundSurf || background.empty()) return;
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
	if (surf == NULL) backgroundSurf = NULL;
	else {
		backgroundSurf = zoomSurface(surf, width / surf->w, height / surf->h, 1);
		SDL_FreeSurface(surf);
	}
}

void CSong::unloadCover() {
	if (coverSurf) SDL_FreeSurface(coverSurf);
	coverSurf = NULL;
}

void CSong::unloadBackground() {
	if (backgroundSurf) SDL_FreeSurface(backgroundSurf);
	backgroundSurf = NULL;
}

bool operator<(CSong const& l, CSong const& r) {
	if (l.artist != r.artist) return l.artist < r.artist;
	if (l.title != r.title) return l.title < r.title;
	return l.filename < r.filename;
	// If filenames are identical, too, the songs are considered the same.
}

CSongs::CSongs(std::set<std::string> const& songdirs): m_songdirs(songdirs), m_current(), m_order() {
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

CSongs::~CSongs() {
	SDL_FreeSurface(surface_nocover);
}

void CSongs::reload() {
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
				songs.insert(new CSong(path + "/", txtfilename));
			} catch (std::exception& e) {
				std::cout << "FAIL\n    " << e.what() << std::endl;
			}
		}
		std::cout << "\r\x1B[K" << std::flush;
		globfree(&_glob);
	}
	m_songs.swap(songs);
	setFilter("");
}

class CSongs::RestoreSel {
	CSongs& m_s;
	CSong* m_sel;
  public:
	RestoreSel(CSongs& s): m_s(s), m_sel(s.empty() ? NULL : &s.current()) {}
	~RestoreSel() {
		m_s.random();
		if (!m_sel) return;
		filtered_t& f = m_s.m_filtered;
		filtered_t::iterator it = std::find(f.begin(), f.end(), m_sel);
		if (it != f.end()) m_s.m_current = it - f.begin();
	}
};

void CSongs::random() {
	m_current = empty() ? 0 : std::rand() % m_filtered.size();
}

void CSongs::setFilter(std::string const& val) {
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
	sortChange(0);
}

class CmpByField {
	std::string CSong::* m_field;
  public:
	CmpByField(std::string CSong::* field): m_field(field) {}
	bool operator()(CSong const& left , CSong const& right) {
		if (left.*m_field == right.*m_field) return left < right;
		return left.*m_field < right.*m_field;
	}
	bool operator()(CSong const* left , CSong const* right) {
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

std::string CSongs::sortDesc() const {
	std::string str = order[m_order];
	if (!empty()) {
		if (m_order == 2) str += " (" + current().edition + ")";
		if (m_order == 3) str += " (" + current().genre + ")";
		if (m_order == 4) str += " (" + current().path + ")";
	}
	return str;
}

void CSongs::sortChange(int diff) {
	m_order = (m_order + diff) % orders;
	if (m_order < 0) m_order += orders;
	RestoreSel restore(*this);
	switch (m_order) {
	  case 0: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&CSong::title)); break;
	  case 1: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&CSong::artist)); break;
	  case 2: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&CSong::edition)); break;
	  case 3: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&CSong::genre)); break;
	  case 4: std::sort(m_filtered.begin(), m_filtered.end(), CmpByField(&CSong::path)); break;
	  default: throw std::logic_error("Internal error: unknown sort order in CSongs::sortChange");
	}
}

