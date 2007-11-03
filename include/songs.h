#ifndef __SONGS_H__
#define __SONGS_H__

#include "../config.h"
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/scoped_ptr.hpp>
#include <set>
#include <string>
#include <vector>

class MusicalScale {
	double m_baseFreq;
	static const int m_baseId = 33;
  public:
	MusicalScale(double baseFreq = 440.0): m_baseFreq(baseFreq) {}
	std::string getNoteStr(double freq) const;
	unsigned int getNoteNum(int id) const;
	bool isSharp(int id) const;
	double getNoteFreq(int id) const;
	int getNoteId(double freq) const;
	double getNote(double freq) const;
	double getNoteOffset(double freq) const;
};

struct Note {
	double begin, end;
	enum Type { FREESTYLE = 'F', NORMAL = ':', GOLDEN = '*', SLEEP = '-'} type;
	int note;
	std::string syllable;
	double diff(double n) const;
	double maxScore() const;
	double score(double freq, double b, double e) const;
  private:
	double scoreMultiplier(double n) const;
};

class SongParser;

class Song: boost::noncopyable {
  public:
	friend class SongParser;
	Song(std::string const& path, std::string const& filename);
	~Song() {
		unloadBackground();
		unloadCover();
	}
	// Temporary score calculation system
	void reset();
	void update(double time, double freq);
	int getScore() const { return 10000 * m_score; }
	bool parseField(std::string const& line);
	std::string str() const { return title + " by " + artist; }
	SDL_Surface* getCover() { loadCover(); return m_coverSurf; }
	SDL_Surface* getBackground() { loadBackground(); return m_backgroundSurf; }
	void loadBackground();
	void loadCover();
	void unloadBackground();
	void unloadCover();
	typedef std::vector<Note> notes_t;
	notes_t notes;
	int noteMin, noteMax;
	std::string path;
	std::string filename;
	std::vector<std::string> category;
	std::string genre;
	std::string edition;
	std::string title;
	std::string artist;
	std::string text;
	std::string creator;
	std::string mp3;
	std::string cover;
	std::string background;
	std::string video;
	double videoGap;
	double start;
	MusicalScale scale;
  private:
	SDL_Surface* m_coverSurf;
	SDL_Surface* m_backgroundSurf;
	double m_scoreFactor; // Normalization factor for the scoring system
	// Temporary score calculation system
	double m_score;
	double m_scoreTime;
	notes_t::const_iterator m_scoreIt;
};

bool operator<(Song const& l, Song const& r);

class Songs {
	std::set<std::string> m_songdirs;
  public:
	Songs(std::set<std::string> const& songdirs);
	~Songs();
	void reload();
	Song& operator[](std::vector<Song*>::size_type pos) { return *m_filtered[pos]; }
	int size() const { return m_filtered.size(); };
	int empty() const { return m_filtered.empty(); };
	void advance(int diff) {
		m_current = (m_current + diff) % int(m_filtered.size());
		if (m_current < 0) m_current += m_filtered.size();
	}
	int currentId() const { return m_current; }
	Song& current() { return *m_filtered[m_current]; }
	Song const& current() const { return *m_filtered[m_current]; }
	void setFilter(std::string const& regex);
	std::string sortDesc() const;
	void random();
	void sortChange(int diff);
	void parseFile(Song& tmp);
	SDL_Surface* getEmptyCover() { return surface_nocover; }
  private:
	class RestoreSel;
	typedef boost::ptr_set<Song> songlist_t;
	songlist_t m_songs;
	typedef std::vector<Song*> filtered_t;
	filtered_t m_filtered;
	int m_current;
	int m_order;
	SDL_Surface* surface_nocover;
	void sort_internal();
};

#endif

