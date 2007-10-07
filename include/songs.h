#ifndef __SONGS_H__
#define __SONGS_H__

#include "../config.h"
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/scoped_ptr.hpp>
#include <set>
#include <string>
#include <vector>

struct TBpm {
	float bpm;
	float start;
};

struct TScore {
	std::string name;
	int hits;
	int total;
	int score;
	std::string length;
};

struct Note {
	Note(std::string const& line, int* relShift);
	enum Type { FREESTYLE = 'F', NORMAL = ':', GOLDEN = '*', SLEEP = '-'} type;
	int timestamp;
	int length;
	int note;
	int curMaxScore;
	std::string syllable;
};

class CSong: boost::noncopyable {
  public:
	CSong(std::string const& path, std::string const& filename);
	~CSong() {
		unloadBackground();
		unloadCover();
	}
	bool parseField(std::string const& line);
	std::string str() const { return title + " by " + artist; }
	unsigned int index;
	std::string path;
	std::string filename;
	std::vector<std::string> category;
	std::string genre;
	std::string edition;
	std::string title;
	std::string artist;
	std::string text;
	std::string creator;
	std::string cover;
	SDL_Surface* getCover() { loadCover(); return coverSurf; }
	SDL_Surface* volatile coverSurf;
	std::string mp3;
	std::string background;
	SDL_Surface* backgroundSurf;
	std::string video;
	float videoGap;
	int noteGap;
	float start;
	int end;
	bool relative;
	std::vector<TBpm> bpm;
	float gap;
	TScore score[3];
	int noteMin;
	int noteMax;
	std::vector<Note> notes;
	bool visible;
	bool main;
	int maxScore;
	void loadBackground();
	void loadCover();
	void unloadBackground();
	void unloadCover();
};

bool operator<(CSong const& l, CSong const& r);

class CSongs {
	std::set<std::string> m_songdirs;
  public:
	CSongs(std::set<std::string> const& songdirs);
	~CSongs();
	void reload();
	CSong& operator[](std::vector<CSong*>::size_type pos) { return *m_filtered[pos]; }
	int size() const { return m_filtered.size(); };
	int empty() const { return m_filtered.empty(); };
	void advance(int diff) {
		m_current = (m_current + diff) % int(m_filtered.size());
		if (m_current < 0) m_current += m_filtered.size();
	}
	int currentId() const { return m_current; }
	CSong& current() { return *m_filtered[m_current]; }
	CSong const& current() const { return *m_filtered[m_current]; }
	void setFilter(std::string const& regex);
	std::string sortDesc() const;
	void random();
	void sortChange(int diff);
	void parseFile(CSong& tmp);
	SDL_Surface* getEmptyCover() { return surface_nocover; }
  private:
	class RestoreSel;
	typedef boost::ptr_set<CSong> songlist_t;
	songlist_t m_songs;
	typedef std::vector<CSong*> filtered_t;
	filtered_t m_filtered;
	int m_current;
	int m_order;
	SDL_Surface* surface_nocover;
};

#endif

