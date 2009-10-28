#include "player.hh"

#include "engine.hh" // just for Engine::TIMESTEP

void Player::update() {
	if (m_pos == m_pitch.size()) return; // End of song already
	double beginTime = Engine::TIMESTEP * m_pos;
	Tone const* t = m_analyzer.findTone();
	if (t) {
		m_activitytimer = 1000;
		m_pitch[m_pos++] = std::make_pair(t->freq, t->stabledb);
	} else {
		if (m_activitytimer > 0) --m_activitytimer;
		m_pitch[m_pos++] = std::make_pair(getNaN(), -getInf());
	}
	double endTime = Engine::TIMESTEP * m_pos;
	while (m_scoreIt != m_song.notes.end()) {
		if (t) {
			// Add score
			double score_addition = m_song.m_scoreFactor * m_scoreIt->score(m_song.scale.getNote(t->freq), beginTime, endTime);
			m_score += score_addition;
			m_lineScore += score_addition;
		}
		// If a row of lyrics ends, calculate how well it went
		if (m_scoreIt->type == Note::SLEEP) {
			if (m_maxLineScore == 0) { // Has the maximum already been calculated for this SLEEP?
				m_prevLineScore = m_lineScore;
				// Calculate max score of the completed row
				Notes::const_reverse_iterator maxScoreIt(m_scoreIt);
				while (maxScoreIt != m_song.notes.rend() && maxScoreIt->type != Note::SLEEP) {
					m_maxLineScore += m_song.m_scoreFactor * maxScoreIt->maxScore();
					maxScoreIt++;
				}
				if (m_maxLineScore > 0) {
					m_prevLineScore /= m_maxLineScore;
					m_feedbackFader.setValue(1.0);
				} else {
					m_prevLineScore = -1;
				}
				m_lineScore = 0;
			}
		} else {
			m_maxLineScore = 0; // Not in SLEEP note anymore, so reset maximum
		}
		if (endTime < m_scoreIt->end) break;
		++m_scoreIt;
	}
	m_score = clamp(m_score, 0.0, 1.0);
}

