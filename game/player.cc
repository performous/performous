#include "player.hh"
#include "song.hh"
#include "engine.hh" // just for Engine::TIMESTEP

Player::Player(VocalTrack& vocal, Analyzer& analyzer, size_t frames):
	  m_vocal(vocal), m_analyzer(analyzer), m_pitch(frames, std::make_pair(getNaN(),
	  -getInf())), m_pos(), m_score(), m_noteScore(), m_lineScore(), m_maxLineScore(),
	  m_prevLineScore(-1), m_feedbackFader(0.0, 2.0), m_activitytimer(),
	  m_scoreIt(m_vocal.notes.begin())
{
	// Initialize note powers
	for (Notes::const_iterator it = m_vocal.notes.begin(); it != m_vocal.notes.end(); ++it) it->power = 0.0f;
	// Assign colors
	m_color = MicrophoneColor::get(m_analyzer.getId());
}

void Player::update() {
	if (m_pos == m_pitch.size()) return; // End of song already
	double beginTime = Engine::TIMESTEP * m_pos;
	// Get the currently sung tone and store it in player's pitch data (also control inactivity timer)
	Tone const* t = m_analyzer.findTone();
	if (t) {
		m_activitytimer = 1000;
		m_pitch[m_pos++] = std::make_pair(t->freq, t->stabledb);
	} else {
		if (m_activitytimer > 0) --m_activitytimer;
		m_pitch[m_pos++] = std::make_pair(getNaN(), -getInf());
	}
	double endTime = Engine::TIMESTEP * m_pos;
	// Iterate over all the notes that are considered for this timestep
	while (m_scoreIt != m_vocal.notes.end()) {
		if (endTime < m_scoreIt->begin) break;  // The note begins later than on this timestep
		// If tone was detected, calculate score
		m_scoreIt->power *= std::pow(0.05, m_scoreIt->clampDuration(beginTime, endTime));  // Fade glow
		if (t) {
			double note = MusicalScale(m_vocal.scale).setFreq(t->freq).getNote();
			// Add score
			double score_addition = m_vocal.m_scoreFactor * m_scoreIt->score(note, beginTime, endTime);
			m_score += score_addition;
			m_noteScore += score_addition;
			m_lineScore += score_addition;
			// Add power if already on the note
			m_scoreIt->power = std::max(m_scoreIt->power, m_scoreIt->powerFactor(note));
		}
		// If a row of lyrics ends, calculate how well it went
		if (m_scoreIt->type == Note::SLEEP) {
			calcRowRank();
		} else {
			m_maxLineScore = 0; // Not in SLEEP note anymore, so reset maximum
		}
		if (endTime < m_scoreIt->end) break;  // The note continues past this timestep
		// Check if we got a star
		if ((m_scoreIt->type == Note::NORMAL || m_scoreIt->type == Note::SLIDE || m_scoreIt->type == Note::GOLDEN || m_scoreIt->type == Note::GOLDEN2)
		  && (m_noteScore / m_vocal.m_scoreFactor / m_scoreIt->maxScore() > 0.8)) {
			m_scoreIt->stars.push_back(m_color);
		}
		m_noteScore = 0; // Reset noteScore as we are moving on to the next one
		m_scoreIt->power = 0.0; // Remove glow
		++m_scoreIt;
	}
	if (m_scoreIt == m_vocal.notes.end()) calcRowRank();
	m_score = clamp(m_score, 0.0, 1.0);
}

void Player::calcRowRank() {
	if (m_maxLineScore == 0) { // Has the maximum already been calculated for this SLEEP?
		m_prevLineScore = m_lineScore;
		// Calculate max score of the completed row
		Notes::const_reverse_iterator maxScoreIt(m_scoreIt);
		// NOTE: MacOSX needs the following cast to compile correctly
		// it is related to the fact that OSX default compiler is 4.0.1 that is buggy when not casting
		while ((maxScoreIt != static_cast<Notes::const_reverse_iterator>(m_vocal.notes.rend())) && (maxScoreIt->type != Note::SLEEP)) {
			m_maxLineScore += m_vocal.m_scoreFactor * maxScoreIt->maxScore();
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
}

PlayerItem::PlayerItem(PlayerId id) 
: id(id)
{
}
