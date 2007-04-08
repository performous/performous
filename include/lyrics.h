#ifndef __LYRICS_H__
#define __LYRICS_H__

#include <../config.h>

#include <songs.h>


/**
 * Song lyrics class. Stores the lyrics, and finds out when they should be sung
 */
class CLyrics {
	public:
	CLyrics( std::vector <TNote *> _lyrics , float _gap , float _bpm );
	~CLyrics();
	/** 
	 * Return what has been sung in the current sentence
	 */
	char * getSentencePast();
	/**
	 * Return the current syllable to be sung now
	 */
	char * getSentenceNow();
	/**
	 * Return the rest of the syllables in the current sentence
	 */
	char * getSentenceFuture();
	/**
	 * Return the next sentence to be displayed under the current sentence
	 */
	char * getSentenceNext();
	/**
	 * Return the whole current sentence (Past + Now + Future)
	 */
	char * getSentenceWhole();
	/**
	 * Return the current sang note
	 */
	TNote * getCurrentNote();
	/**
	 * Return the current sang sentence structure
	 */
	std::vector <TNote *> getCurrentSentence();
	/** 
	 * update the content of the sentence strings
	 */
	void updateSentences( unsigned int timestamp );
	private:
	/**
	 * Convert a beat number to a timestamp (according to GAP and BPM)
	 */
	unsigned int getTimestampFromBeat( unsigned int beat );
	/**
	 * Get the start timestamp of a sentence
	 */
	unsigned int getStartTime( int sentence );
	/**
	 * Get the end timestamp of a sentence
	 */
	unsigned int getEndTime( int sentence );
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
