#include "engine.hh"

const double Engine::TIMESTEP = 0.01;

void Player::update() {
	Tone const* t = m_analyzer.findTone();
	if (t) {
		m_activitytimer = 1000;
		m_scoreIt = m_song.notes.begin(); // TODO: optimize
		m_pitch.push_back(std::make_pair(t->freq, t->stabledb));
		double beginTime = Engine::TIMESTEP * (m_pitch.size() - 1);
		double endTime = beginTime + 0.01;
		while (m_scoreIt != m_song.notes.end()) {
			m_score += m_song.m_scoreFactor * m_scoreIt->score(m_song.scale.getNote(t->freq), beginTime, endTime);
			if (endTime < m_scoreIt->end) break;
			++m_scoreIt;
		}
		m_score = clamp(m_score, 0.0, 1.0);
	} else {
		if (m_activitytimer > 0) --m_activitytimer;
		m_pitch.push_back(std::make_pair(getNaN(), -getInf()));
	}
}

