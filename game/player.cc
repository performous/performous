#include "player.hh"

#include "engine.hh" // just for Engine::TIMESTEP

void Player::update() {
	if (m_pos == m_pitch.size()) return; // End of song already
	Tone const* t = m_analyzer.findTone();
	if (t) {
		m_activitytimer = 1000;
		double beginTime = Engine::TIMESTEP * m_pos;
		m_pitch[m_pos++] = std::make_pair(t->freq, t->stabledb);
		double endTime = Engine::TIMESTEP * m_pos;
		while (m_scoreIt != m_song.notes.end()) {
			m_score += m_song.m_scoreFactor * m_scoreIt->score(m_song.scale.getNote(t->freq), beginTime, endTime);
			if (endTime < m_scoreIt->end) break;
			++m_scoreIt;
		}
		m_score = clamp(m_score, 0.0, 1.0);
	} else {
		if (m_activitytimer > 0) --m_activitytimer;
		m_pitch[m_pos++] = std::make_pair(getNaN(), -getInf());
	}
}

