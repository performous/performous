#ifndef __SONGS_H__
#define __SONGS_H__

#include <vector>
#include <texture.h>

typedef struct _SBpm {
	float bpm;
	float start;
} TBpm ;

typedef struct _SScore {
	char * name;
	int score;
	char * length;
} TScore ;

#define TYPE_NOTE_SING  0
#define TYPE_NOTE_SLEEP 1

typedef struct _SNote {
	int type;
	int timestamp;
	int length;
	int note;
	char * syllable;
} TNote;

class CSong {
	public:
	CSong();
	~CSong() {};
	void parseFile( void );
	
	bool operator< (const CSong&  right) const;

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
	CSdlTexture * coverTex;
	char * mp3;
	char * background;
	char * video;
	float videoGap;
	int noteGap;
	float start;
	int end;
	bool relative;
	std::vector <TBpm> bpm;
	float gap;
	TScore score[3];
	std::vector <TNote *> notes;
	bool visible;
	bool main;
	int orderNum;
	int orderType;
};

class CSongs {
	public:
		CSongs();
		~CSongs();
		CSong * getSong( int i ) {return songs[i];};
		int nbSongs( void ) {return songs.size();};
		void parseFile( CSong * tmp );
		void sortByEdition( void );
		void sortByGenre( void );
		void sortByTitle( void );
		void sortByArtist( void );
		int getOrder( void ) {return order;};
	private:
		std::vector <CSong*> songs;
		int selected;
		int order;
		int category;
		CSdlTexture * texture_nocover;
};

#endif
