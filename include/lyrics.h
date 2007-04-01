#ifndef __LYRICS_H__
#define __LYRICS_H__

#include <../config.h>

#include <songs.h>

class CLyrics {
	public:
	CLyrics( std::vector <TNote *> _lyrics , float _gap , float _bpm );
	~CLyrics();
	char * getSentencePast();
	char * getSentenceNow();
	char * getSentenceFuture();
	char * getSentenceNext();
	char * getSentenceWhole();
	TNote * getCurrentNote();
	std::vector <TNote *> getCurrentSentence();
	void updateSentences( unsigned int timestamp );
	private:
	unsigned int getTimestampFromBeat( unsigned int beat );
	std::vector <TNote *> lyrics;
	std::vector < std::vector <TNote *> > formatedLyrics;
	char sentencePast[1024];
	char sentenceNow[1024];
	char sentenceFuture[1024];
	char sentenceNext[1024];
	char sentenceWhole[1024];
	int lastSyllableIndex;
	int lastSentenceIndex;
	float gap;
	float bpm;
};

#endif
