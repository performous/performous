#include <lyrics.h>

CLyrics::CLyrics(std::vector<Note> const& lyrics, float gap ,float bpm):
  m_lyrics(lyrics),
  m_lastSyllableIdx(-1),
  m_lastSentenceIdx(-1),
  m_gap(gap),
  m_bpm(bpm)
{
	std::vector<Note> tmp;
	unsigned int size = lyrics.size();
	for (unsigned int i = 0; i < size; ++i) {
		while(i < size && lyrics[i].type == Note::SLEEP) i++;
		while(i < size && lyrics[i].type != Note::SLEEP) {
			tmp.push_back(lyrics[i]);
			i++;
		}
		if (!tmp.empty()) m_formatted.push_back(tmp);
		tmp.clear();
	}
}

std::vector<Note> CLyrics::getCurrentSentence() {
	return m_lastSentenceIdx != -1 ? m_formatted[m_lastSentenceIdx] : std::vector<Note>();
}

Note* CLyrics::getCurrentNote() {
	return (m_lastSentenceIdx != -1 && m_lastSyllableIdx != -1) ?
	  &m_formatted[m_lastSentenceIdx][m_lastSyllableIdx] : NULL;
}

void CLyrics::updateSentences(unsigned int timestamp) {
	// If the sentences shouldn't change, do nothing
	if (m_lastSyllableIdx != -1 && m_lastSentenceIdx != -1
	  && m_lastSentenceIdx < (int) m_formatted.size()
	  && m_lastSyllableIdx < (int) m_formatted[m_lastSentenceIdx].size()
	  && timestamp >= getTimestampFromBeat(m_formatted[m_lastSentenceIdx][m_lastSyllableIdx].timestamp)
	  && timestamp <= getTimestampFromBeat(m_formatted[m_lastSentenceIdx][m_lastSyllableIdx].timestamp + m_formatted[m_lastSentenceIdx][m_lastSyllableIdx].length)) return;
	// sentence changed, recompute it
	// If we are further than the m_last time (no rewind) (optimisation)
	unsigned int i = (m_lastSentenceIdx != -1 && timestamp > getStartTime(m_lastSentenceIdx)) ? m_lastSentenceIdx : 0;

	// For all the detected sentences, find the first that have not yet ended
	for (; i < m_formatted.size(); ++i) {
		// If we're between the end of the m_last sentence and the end of the current sentence
		if (timestamp > getEndTime(i-1) && timestamp <= getEndTime(i)) {
			m_past.clear();
			m_now.clear();
			m_future.clear();
			for (unsigned int j = 0; j < m_formatted[i].size(); ++j) {
				if (timestamp > getTimestampFromBeat(m_formatted[i][j].timestamp + m_formatted[i][j].length)) {
					m_past += m_formatted[i][j].syllable;
				} else if (timestamp < getTimestampFromBeat(m_formatted[i][j].timestamp)) {
					m_future += m_formatted[i][j].syllable;
				} else {
					m_lastSyllableIdx = j;
					m_now += m_formatted[i][j].syllable;
				}
			}
			// If we have change of sentence, we rebuild the next sentence
			if (m_lastSentenceIdx == -1 || (unsigned int)m_lastSentenceIdx != i) {
				m_lastSentenceIdx = i;
				m_next.clear();
				if (i != m_formatted.size() - 1) {
					for (unsigned int j = 0; j < m_formatted[i+1].size(); ++j)
					  m_next += m_formatted[i+1][j].syllable;
				}
			}
			// No need to go further in the song
			break;
		}
	}

}

unsigned int CLyrics::getTimestampFromBeat(unsigned int beat) {
	return (unsigned int) ((beat * 60 * 1000) / (m_bpm * 4) + m_gap);
}

unsigned int CLyrics::getStartTime(int sentence) {
	if (sentence < 0) return 0;
	if ((unsigned int)sentence >= m_formatted.size()) return UINT_MAX;
	return getTimestampFromBeat(m_formatted[sentence][0].timestamp);
}

unsigned int CLyrics::getEndTime(int sentence) {
	if (sentence < 0) return 0;
	if ((unsigned int) sentence >= m_formatted.size()) return UINT_MAX;
	return getTimestampFromBeat(m_formatted[sentence][m_formatted[sentence].size()-1].timestamp + m_formatted[sentence][m_formatted[sentence].size()-1].length);
}

