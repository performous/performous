#ifndef __LYRICS_H__
#define __LYRICS_H__

#include <../config.h>

#include <songs.h>


/**
 * Song lyrics class. Stores the lyrics, and finds out when they should be sung
 */
class CLyrics {
  public:
	CLyrics(std::vector<TNote> const& lyrics, float gap, float bpm);
	/** 
	 * Return what has been sung in the current sentence
	 */
	std::string getSentencePast() { return m_past; }
	/**
	 * Return the current syllable to be sung now
	 */
	std::string getSentenceNow() { return m_now; }
	/**
	 * Return the rest of the syllables in the current sentence
	 */
	std::string getSentenceFuture() { return m_future; }
	/**
	 * Return the next sentence to be displayed under the current sentence
	 */
	std::string getSentenceNext() { return m_next; }
	/**
	 * Return the whole current sentence (Past + Now + Future)
	 */
	std::string getSentenceWhole() { return m_past + m_now + m_future; }
	/**
	 * Return the current sang note
	 */
	TNote getCurrentNote();
	/**
	 * Return the current sang sentence structure
	 */
	std::vector<TNote> getCurrentSentence();
	/** 
	 * update the content of the sentence strings
	 */
	void updateSentences(unsigned int timestamp);
  private:
	/**
	 * Convert a beat number to a timestamp (according to GAP and BPM)
	 */
	unsigned int getTimestampFromBeat(unsigned int beat);
	/**
	 * Get the start timestamp of a sentence
	 */
	unsigned int getStartTime(int sentence);
	/**
	 * Get the end timestamp of a sentence
	 */
	unsigned int getEndTime(int sentence);
	
	std::string m_past, m_now, m_future, m_next;
	std::vector<TNote> m_lyrics;
	std::vector<std::vector<TNote> > m_formatted;
	int m_lastSyllableIdx;
	int m_lastSentenceIdx;
	float m_gap;
	float m_bpm;
};

#endif
