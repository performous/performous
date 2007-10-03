#include <boost/lexical_cast.hpp>
#include <boost/thread/thread.hpp>
#include <glob.h>
#include <songs.h>
#include <screen.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

void CSong::parseFile() {
	int relativeShift = 0;
	maxScore = 0;
	std::string file = std::string(path) + "/" + filename;
	std::ifstream f(file.c_str());
	for (std::string line; std::getline(f, line); ) {
		if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
		if (line.empty() || line[0] == '#') continue;
		if (line[0] == 'E') break;
		std::istringstream iss(line);
		switch (iss.get()) {
		  case 'F':
		  case ':':
		  case '*':
			{
				TNote tmp;
				if (line[0] == 'F') tmp.type = TYPE_NOTE_FREESTYLE;
				if (line[0] == '*') tmp.type = TYPE_NOTE_GOLDEN;
				if (line[0] == ':') tmp.type = TYPE_NOTE_NORMAL;
				if (!(iss >> tmp.timestamp >> tmp.length >> tmp.note)) throw std::runtime_error("Invalid note line format");
				if (tmp.length == 0) break; // 0-length notes are ignored
				tmp.timestamp += relativeShift;
				if (iss.get() == ' ') std::getline(iss, tmp.syllable);
				noteMin = std::min(noteMin, tmp.note);
				noteMax = std::max(noteMax, tmp.note);
				maxScore += tmp.length * tmp.type;
				tmp.curMaxScore = maxScore;
				notes.push_back(tmp);
				break;
			}
		  case '-':
			{
				TNote tmp;
				int timestamp = 0;
				int sleep_end;
				tmp.type = TYPE_NOTE_SLEEP;
				if (iss >> timestamp) tmp.length = (iss >> sleep_end) ? sleep_end - timestamp : 0;
				tmp.timestamp = relativeShift + timestamp;
				if (relative) relativeShift += timestamp + tmp.length;
				tmp.curMaxScore = maxScore;
				notes.push_back(tmp);
				break;
			}
		}
	}
	// Adjust negative notes
	if (noteMin <= 0) {
		unsigned int shift = (((noteMin*-1)%12)+1)*12;
		noteMin += shift;
		noteMax += shift;
		for(unsigned i = 0; i < notes.size(); ++i) notes[i].note += shift;
	}
}

CSong::CSong():
	path(),
	filename(),
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
	noteMin(256),
	noteMax(-256)
{}

void CSong::loadCover() {
	if (coverSurf || cover.empty()) return;
	double width = CScreenManager::getSingletonPtr()->getWidth();
	double height = CScreenManager::getSingletonPtr()->getHeight();
	std::string file = std::string(path) + "/" + cover;
	std::string::size_type extpos = file.rfind('.');
	std::string ext = (extpos == std::string::npos) ? "" : file.substr(extpos);
	SDL_RWops* rwop = SDL_RWFromFile(file.c_str(), "rb");
	SDL_Surface* surf = NULL;
	if (ext == ".jpg" || ext == ".JPG" || ext == ".jpeg") surf = IMG_LoadJPG_RW(rwop);
	else if (ext == ".png" || ext == ".PNG") surf = IMG_LoadPNG_RW(rwop);
	//else std::cout << "Cover image file " << file << " has unknown extension" << std::endl;
	if (rwop) SDL_RWclose(rwop);
	if (surf == NULL) coverSurf = NULL;
	else {
		// Here we want to have cover of 256x256 in 800x600 and scale it if the resolution is different
		double w = width * 256 / 800;
		double h = height * 256 / 600;
		coverSurf = zoomSurface(surf, w / surf->w, h / surf->h, 1);
		SDL_FreeSurface(surf);
	}
}

void CSong::loadBackground() {
	if (backgroundSurf || background.empty()) return;
	double width = CScreenManager::getSingletonPtr()->getWidth();
	double height = CScreenManager::getSingletonPtr()->getHeight();
	std::string file = std::string(path) + "/" + background;
	std::string::size_type extpos = file.rfind('.');
	std::string ext = (extpos == std::string::npos) ? "" : file.substr(extpos);
	SDL_RWops* rwop = SDL_RWFromFile(file.c_str(), "rb");
	SDL_Surface* surf = NULL;
	if (ext == ".jpg" || ext == ".JPG" || ext == ".jpeg") surf = IMG_LoadJPG_RW(rwop);
	else if (ext == ".png" || ext == ".PNG") surf = IMG_LoadPNG_RW(rwop);
	//else std::cout << "Background image file " << file << " has unknown extension" << std::endl;
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
	SDL_Surface* surface_nocover_tmp = IMG_LoadPNG_RW(rwop_nocover);
	int w = CScreenManager::getSingletonPtr()->getWidth()*256/800;
	int h = CScreenManager::getSingletonPtr()->getHeight()*256/600;
	surface_nocover = zoomSurface(surface_nocover_tmp,(double)w/surface_nocover_tmp->w,(double)h/surface_nocover_tmp->h,1);
	SDL_FreeSurface(surface_nocover_tmp);
	if (rwop_nocover) SDL_RWclose(rwop_nocover);
	if (surface_nocover == NULL) throw std::runtime_error("Cannot load " + file);
	reload();
}

CSongs::~CSongs() {
	SDL_FreeSurface(surface_nocover);
}

class CSongs::SongLoader {
	CSongs& m_self;
  public:
	SongLoader(CSongs& self): m_self(self) {}
	void operator()() {
		boost::mutex::scoped_lock l(m_self.m_mutex);
		songlist_t& s = m_self.m_songs;
		boost::progress_display pd(s.size() + 1);
		for (songlist_t::iterator it = s.begin(); ++pd, it != s.end(); ++it) it->loadCover();
	}
};

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
			CSong* tmp = new CSong();
			try {
				tmp->path = path + "/";
				tmp->filename = txtfilename;
				parseFile(*tmp);
				tmp->parseFile();
				songs.push_back(tmp);
			}
			catch (...) {
				std::cout << "FAIL" << std::endl;
				delete tmp;
			}
		}
		std::cout << "\r\x1B[K" << std::flush;
		globfree(&_glob);
	}
	boost::mutex::scoped_lock l(m_mutex);
	m_songs.swap(songs);
	setFilter("");
	boost::thread(SongLoader(*this));
}

namespace {
	double toDouble(std::string str) {
		std::replace(str.begin(), str.end(), ',', '.'); // Fix decimal separators
		return boost::lexical_cast<double>(str);
	}
	bool toBool(std::string const& str) {
		char c = str.empty() ? 0 : str[0];
		return c == 'y' || c == 'Y' || c == '1';
	}
}

void CSongs::parseFile(CSong& tmp) {
	std::string file = tmp.path + tmp.filename;
	std::ifstream f(file.c_str());
	if (!f.is_open()) throw std::runtime_error("Cannot open " + file);
	std::string line;
	while (std::getline(f, line)) {
		if (line.empty() || line[0] != '#') continue;
		std::string::size_type pos = line.find(':');
		if (pos == std::string::npos) throw std::runtime_error("Invalid format in " + file);
		std::string key = line.substr(1, pos - 1);
		std::string value = line.substr(pos + 1);
		if (key == "TITLE") tmp.title = value;
		else if (key == "ARTIST") tmp.artist = value;
		else if (key == "EDITION") tmp.edition = value;
		else if (key == "GENRE") tmp.genre = value;
		else if (key == "CREATOR") tmp.creator = value;
		else if (key == "COVER") tmp.cover = value;
		else if (key == "MP3") tmp.mp3 = value;
		else if (key == "VIDEO") tmp.video = value;
		else if (key == "BACKGROUND") tmp.background = value;
		else if (key == "VIDEOGAP") tmp.videoGap = toDouble(value);
		else if (key == "RELATIVE") tmp.relative = toBool(value);
		else if (key == "GAP") tmp.gap = toDouble(value);
		else if (key == "BPM") {
			TBpm bpm;
			bpm.start = 0.0;
			bpm.bpm = toDouble(value);
			tmp.bpm.push_back(bpm);
		}
	}
	if (tmp.title.empty() || tmp.artist.empty()) throw std::runtime_error("Required fields missing in " + file);
}

class CSongs::RestoreSel {
	CSongs& m_s;
	CSong* m_sel;
  public:
	RestoreSel(CSongs& s): m_s(s), m_sel(s.empty() ? NULL : &s.current()) {}
	~RestoreSel() {
		if (!m_sel) return;
		filtered_t& f = m_s.m_filtered;
		filtered_t::iterator it = std::find(f.begin(), f.end(), m_sel);
		if (it != f.end()) m_s.m_current = it - f.begin();
	}
};

void CSongs::setFilter(std::string const& val) {
	RestoreSel restore(*this);
	m_filtered.clear();
	for (songlist_t::iterator it = m_songs.begin(); it != m_songs.end(); ++it) {
		if (val.empty() || it->str().find(val) != std::string::npos) m_filtered.push_back(&*it);
	}
	sortChange(0);
	m_current = 0;
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
};

static const int orders = sizeof(order) / sizeof(*order);

std::string CSongs::sortDesc() const {
	std::string str = order[m_order];
	if (!empty()) {
		if (m_order == 2) str += " (" + current().edition + ")";
		if (m_order == 3) str += " (" + current().genre + ")";
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
	  default: throw std::logic_error("Internal error: unknown sort order in CSongs::sortChange");
	}
}

