#include "lyrics.hh"
#include <limits>

Lyrics::Lyrics(Song::notes_t const& lyrics):
  m_lyrics(lyrics),
  m_lastSyllableIdx(-1),
  m_lastSentenceIdx(-1)
{
	std::vector<Note> tmp;
	for (Song::notes_t::const_iterator it = lyrics.begin(); it != lyrics.end(); ++it) {
		if (it->type != Note::SLEEP) { tmp.push_back(*it); continue; }
		if (!tmp.empty()) { m_formatted.push_back(tmp); tmp.clear(); }
	}
	if (!tmp.empty()) m_formatted.push_back(tmp);
}

std::vector<Note> Lyrics::getCurrentSentence() {
	return m_lastSentenceIdx != -1 ? m_formatted[m_lastSentenceIdx] : std::vector<Note>();
}

Note* Lyrics::getCurrentNote() {
	return (m_lastSentenceIdx != -1 && m_lastSyllableIdx != -1) ?
	  &m_formatted[m_lastSentenceIdx][m_lastSyllableIdx] : NULL;
}

void Lyrics::updateSentences(double timestamp) {
	// If the sentences shouldn't change, do nothing
	if (m_lastSyllableIdx != -1 && m_lastSentenceIdx != -1
	  && m_lastSentenceIdx < (int) m_formatted.size()
	  && m_lastSyllableIdx < (int) m_formatted[m_lastSentenceIdx].size()
	  && timestamp >= m_formatted[m_lastSentenceIdx][m_lastSyllableIdx].begin
	  && timestamp <= m_formatted[m_lastSentenceIdx][m_lastSyllableIdx].end) return;
	// sentence changed, recompute it
	// If we are further than the m_last time (no rewind) (optimisation)
	unsigned int i = (m_lastSentenceIdx != -1 && timestamp > getStartTime(m_lastSentenceIdx)) ? m_lastSentenceIdx : 0;
	// For all the detected sentences, find the first that have not yet ended
	for (; i < m_formatted.size(); ++i) if (timestamp <= getEndTime(i)) break;
	// If we're between the end of the m_last sentence and the end of the current sentence
	m_past.clear();
	m_now.clear();
	m_future.clear();
	m_whole.clear();
	if (i >= m_formatted.size()) {
		m_lastSentenceIdx = -1;
		return;
	}
	for (unsigned int j = 0; j < m_formatted[i].size(); ++j) {
		if (timestamp > m_formatted[i][j].end) {
			m_past.push_back(m_formatted[i][j].syllable);
			m_whole.push_back(m_formatted[i][j].syllable);
		} else if (timestamp < m_formatted[i][j].begin) {
			m_future.push_back(m_formatted[i][j].syllable);
			m_whole.push_back(m_formatted[i][j].syllable);
		} else {
			m_lastSyllableIdx = j;
			m_now.push_back(m_formatted[i][j].syllable);
			m_whole.push_back(m_formatted[i][j].syllable);
		}
	}
	// If we have change of sentence, we rebuild the next sentence
	if (m_lastSentenceIdx == -1 || (unsigned int)m_lastSentenceIdx != i) {
		m_lastSentenceIdx = i;
		m_next.clear();
		if (i < m_formatted.size() - 1) {
			for (unsigned int j = 0; j < m_formatted[i+1].size(); ++j)
			  m_next.push_back(m_formatted[i+1][j].syllable);
		}
	}
}

double Lyrics::getStartTime(int sentence) {
	if (sentence < 0) return 0.0;
	if (std::size_t(sentence) >= m_formatted.size()) return std::numeric_limits<double>::max();
	return m_formatted[sentence][0].begin;
}

double Lyrics::getEndTime(int sentence) {
	if (sentence < 0) return 0.0;
	if (std::size_t(sentence) >= m_formatted.size()) return std::numeric_limits<double>::max();
	return m_formatted[sentence][m_formatted[sentence].size()-1].end;
}

