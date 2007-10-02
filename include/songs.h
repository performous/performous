#ifndef __SONGS_H__
#define __SONGS_H__

#include "../config.h"
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
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

#define TYPE_NOTE_FREESTYLE 0
#define TYPE_NOTE_NORMAL  1
#define TYPE_NOTE_GOLDEN 2
#define TYPE_NOTE_SLEEP 3

struct TNote {
	int type;
	int timestamp;
	int length;
	int note;
	int curMaxScore;
	std::string syllable;
};

class CSong: boost::noncopyable {
  public:
	CSong();
	~CSong() {
		unloadBackground();
		unloadCover();
	}
	void parseFile();
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
	SDL_Surface* coverSurf;
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
	std::vector<TNote> notes;
	bool visible;
	bool main;
	int orderNum;
	int orderType;
	int maxScore;
	void loadBackground();
	void loadCover();
	void unloadBackground();
	void unloadCover();
};

class CSongs {
  public:
	CSongs(std::set<std::string> const& songdirs);
	~CSongs();
	CSong& operator[](std::vector<CSong*>::size_type pos) { return songs[pos]; }
	int nbSongs() { return songs.size(); };
	bool parseFile(CSong& tmp);
	void sortByEdition();
	void sortByGenre();
	void sortByTitle();
	void sortByArtist();
	int getOrder() { return order; };
	SDL_Surface* getEmptyCover() { return surface_nocover; }
  private:
	typedef boost::ptr_vector<CSong> songlist_t;
	songlist_t songs;
	int selected;
	int order;
	int category;
	SDL_Surface* surface_nocover;
};

#endif
