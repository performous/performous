#ifndef __LYRICS_H__
#define __LYRICS_H__

#include "songs.hh"
#include <vector>


/**
 * Song lyrics class. Stores the lyrics, and finds out when they should be sung
 */
class Lyrics {
  public:
	Lyrics(Song::notes_t const& lyrics);
	/** 
	 * Return what has been sung in the current sentence
	 */
	std::vector<std::string> getSentencePast() { return m_past; }
	/**
	 * Return the current syllable to be sung now
	 */
	std::vector<std::string> getSentenceNow() { return m_now; }
	/**
	 * Return the rest of the syllables in the current sentence
	 */
	std::vector<std::string> getSentenceFuture() { return m_future; }
	/**
	 * Return the next sentence to be displayed under the current sentence
	 */
	std::vector<std::string> getSentenceNext() { return m_next; }
	/**
	 * Return the whole current sentence (Past + Now + Future)
	 */
	std::vector<std::string> getSentenceWhole() { return m_whole; }
	/**
	 * Return the current sang note
	 */
	Note* getCurrentNote();
	/**
	 * Return the current sang sentence structure
	 */
	std::vector<Note> getCurrentSentence();
	/** 
	 * update the content of the sentence strings
	 */
	void updateSentences(double timestamp);
  private:
	/**
	 * Get the start timestamp of a sentence
	 */
	double getStartTime(int sentence);
	/**
	 * Get the end timestamp of a sentence
	 */
	double getEndTime(int sentence);
	
	std::vector<std::string> m_past, m_now, m_future, m_next, m_whole;
	Song::notes_t m_lyrics;
	std::vector<std::vector<Note> > m_formatted;
	int m_lastSyllableIdx;
	int m_lastSentenceIdx;
};

#endif
