#ifndef __SONGS_H__
#define __SONGS_H__

#include "../config.h"

typedef struct _SBpm {
	float bpm;
	float start;
} TBpm ;

typedef struct _SScore {
	char * name;
	int hits;
	int total;
	int score;
	char * length;
} TScore ;

#define TYPE_NOTE_FREESTYLE 0
#define TYPE_NOTE_NORMAL  1
#define TYPE_NOTE_GOLDEN 2
#define TYPE_NOTE_SLEEP 3

typedef struct _SNote {
	int type;
	int timestamp;
	int length;
	int note;
	int curMaxScore;
	char * syllable;
} TNote;

class CSong {
	public:
	CSong();
	~CSong() {};
	void parseFile( void );

	unsigned int index;
	char * path;
	char * filename;
	std::vector <char *> category;
	char * genre;
	char * edition;
	char * title;
	char * artist;
	char * text;
	char * creator;
	char * cover;
	SDL_Surface * coverSurf;
	char * mp3;
	char * background;
         SDL_Surface * backgroundSurf;	
        char * video;
	float videoGap;
	int noteGap;
	float start;
	int end;
	bool relative;
	std::vector <TBpm> bpm;
	float gap;
	TScore score[3];
	int noteMin;
	int noteMax;
	std::vector <TNote *> notes;
	bool visible;
	bool main;
	int orderNum;
	int orderType;
        int maxScore;
};

class CSongs {
	public:
		CSongs();
		~CSongs();
		CSong * getSong( unsigned int i );
		int nbSongs( void ) {return songs.size();};
		bool parseFile( CSong * tmp );
		void sortByEdition( void );
		void sortByGenre( void );
		void sortByTitle( void );
		void sortByArtist( void );
		int getOrder( void ) {return order;};
		void loadCover( unsigned int i);
		void loadBackground( unsigned int i);
		void unloadCover( unsigned int i);
		void unloadBackground( unsigned int i);
	private:
		std::vector <CSong*> songs;
		int selected;
		int order;
		int category;
		SDL_Surface * surface_nocover;
};

#endif
