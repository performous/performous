#include "guitarengine.hh"

#include "log.hh"

#include <cmath>
#include <cstdlib>

namespace {
	struct Diff { char const* name; unsigned basepitch; } diffv[] = {
		{ "Kids", 0x3C },
		{ "Easy", 0x3C },
		{ "Medium", 0x48 },
		{ "Hard", 0x54 },
		{ "Expert", 0x60 }
	};
	const double maxTolerance = GuitarEngine::maxTolerance;

	double points(double error) {
		// error    points
		// 150ms      15
		// 100ms      30
		//  50ms      45
		//  30ms      50
		double score = 0.0;
		if (error < maxTolerance) score += 15;
		if (error < 0.1) score += 15;
		if (error < 0.05) score += 15;
		if (error < 0.03) score += 5;
		return score;
	}

	const int streakStarBonus = 500;
	int getNextBigStreak(int prev) { return prev + 50; }
}

unsigned GuitarEngine::basePitch(Difficulty level) { return diffv[to_underlying(level)].basepitch; }

char const* GuitarEngine::difficultyName(Difficulty level) { return diffv[to_underlying(level)].name; }

bool GuitarEngine::hasDifficulty(InstrumentTrack const& track, Difficulty level) {
	const unsigned pads = 5;
	unsigned basepitch = basePitch(level);
	NoteMap const& nm = track.nm;
	unsigned fail = 0;
	for (unsigned fret = 0; fret < pads; ++fret) if (nm.find(basepitch + fret) == nm.end()) ++fail;
	return fail != pads;
}

GuitarEngine::GuitarEngine(Listener& listener, bool drums, bool proMode, bool songHasBRE):
  m_listener(listener),
  m_drums(drums),
  m_proMode(proMode),
  m_songHasBRE(songHasBRE),
  m_level(),
  m_chordIt(m_chords.end()),
  m_dfIt(m_drumfills.end()),
  m_correctness(1.0, 5.0),
  m_starpower(0.0, 0.1),
  m_score(),
  m_scoreFactor(),
  m_starmeter(),
  m_streak(),
  m_longestStreak(),
  m_bigStreak(),
  m_drumfillHits(),
  m_drumfillScore(),
  m_soloTotal(),
  m_soloScore(),
  m_solo(),
  m_hasTomTrack(false),
  m_whammy(0)
{
	for (size_t i = 0; i < max_panels; ++i) {
		m_holds[i] = 0;
		m_pressed[i] = false;
	}
}

bool GuitarEngine::setDifficulty(InstrumentTrack const& track, Difficulty level) {
	if (!hasDifficulty(track, level)) return false;
	m_track = &track;
	Difficulty prevLevel = m_level;
	m_level = level;
	updateChords();
	if (m_chords.size() <= 1) { // If there is only one chord, it's probably b0rked
		m_level = prevLevel;
		updateChords();
		return false;
	}
	return true;
}

void GuitarEngine::resetScore() {
	m_score = 0;
	m_starmeter = 0;
	m_streak = 0;
	m_longestStreak = 0;
	m_bigStreak = 0;
}

unsigned GuitarEngine::noteEvent(Duration const* dur) const {
	auto it = m_notes.find(dur);
	return it == m_notes.end() ? 0 : it->second;
}

double GuitarEngine::missVolumeTarget() const {
	auto missVolumePercent = 0u;
	if (m_level >= Difficulty::KIDS && m_level < Difficulty::COUNT) missVolumePercent = settings.missVolumePercent[to_underlying(m_level)];
	return clamp(static_cast<double>(missVolumePercent) / 100.0);
}

void GuitarEngine::beginFrame(double time) {
	if (!m_drumfills.empty()) updateDrumFill(time); // Drum Fills / BREs
	m_whammy = 0;
}

void GuitarEngine::input(double time, input::Event ev) {
	unsigned buttonId = to_underlying(ev.button.id);
	// Guitar specific actions
	if (!m_drums) {
		if (ev.button == input::ButtonId::GUITAR_GODMODE && ev.pressed()) activateStarpower();
		if (ev.button == input::ButtonId::GUITAR_WHAMMY) m_whammy = (1.0 + ev.value + 2.0*(rand()/double(RAND_MAX))) / 4.0;
		if (buttonId <= m_pads && !ev.pressed()) endHold(buttonId, time);
	}

	// Playing
	if (m_drums) {
		if (ev.pressed() && ev.button.layer() < 8 && ev.button.num() < m_pads) drumHit(time, ev.button.layer(), ev.button.num());
	} else {
		guitarPlay(time, ev);
	}
	if (m_score < 0) m_score = 0;
}

void GuitarEngine::update(double time) {
	// Skip missed chords
	// - Only after we are so much past them that they can no longer be played (maxTolerance)
	// - For chords played or skipped by playing (i.e. play another chord that quickly follows), ++m_chordIt is done elsewhere
	while (m_chordIt != m_chords.end() && m_chordIt->begin + maxTolerance < time) {
		// For guitar, status is 0/1/2 (not played/tapped/picked). For drums, status counts pads hit (0 to polyphony).
		bool chordMissed = m_drums ? (m_chordIt->status < m_chordIt->polyphony) : (m_chordIt->status == 0);
		if (chordMissed) endStreak();
		// Calculate solo total score
		if (m_solo) { m_soloScore += m_chordIt->score; m_soloTotal += static_cast<float>(m_chordIt->polyphony) * static_cast<float>(points(0));
		// Solo just ended?
		} else if (m_soloTotal > 0) {
			m_listener.onSoloEnded(unsigned(m_soloScore / m_soloTotal * 100));
			m_soloScore = 0;
			m_soloTotal = 0;
		}
		m_listener.onChordPassed(time);
		++m_chordIt;
	}
	// Start decreasing correctness instantly if the current note is being played late (don't wait until maxTolerance)
	if (m_chordIt != m_chords.end() && m_chordIt->begin < time && m_chordIt->status == 0) {
		m_correctness.setTarget(missVolumeTarget());
	}
	// Process holds
	if (!m_drums) {
		// FIXME: Why do we have per-fret hold handling, why not just as a part of the current chord?
		// FIXME: Use polyphony from proper chord (which isn't always m_chordIt); now it only counts frets currently held
		unsigned count = 0;
		bool holds = false;
		for (unsigned fret = 0; fret < m_pads; ++fret) {
			if (!m_holds[fret]) continue;
			holds = true;
			PlayEvent& ev = m_events[m_holds[fret] - 1];
			ev.glow.setTarget(1.0, true);
			// Whammy animvalue mangling
			ev.whammy.setTarget(m_whammy);
			if (m_whammy > 0.0f) {
				ev.whammy.move(0.5f);
				if (ev.whammy.get() > 1.0f) ev.whammy.setValue(1.0f);
			}
			// Calculate how long the note was held this cycle and score it
			double last = std::min(time, ev.dur->end);
			double t = last - ev.holdTime;
			if (t < 0) continue;  // FIXME: What is this for, rewinding?
			// Is the hold being played correctly?
			// Just holding the fret button sustains the note
			++count;
			// Score for holding
			m_score += t * 50.0f * m_correctness.get();
			// Whammy fills starmeter much faster
			m_starmeter += static_cast<float>(t * 50.0f * ( (ev.whammy.get() > 0.01f) ? 2.0f : 1.0f ));
			ev.holdTime = time;
			// If end reached, handle it
			if (last == ev.dur->end) endHold(fret, time);
		}
		// Set correctness as a percentage of chord being held
		// m_chordIt is end() while the sustain of the song's last chord is still being held
		if (holds && m_chordIt != m_chords.end()) m_correctness.setTarget(clamp(double(count) / m_chordIt->polyphony));  // FIXME: polyphony doesn't seem to be set correctly for guitar tracks (workaround by clamping)
	}
	// Update solo status
	m_solo = false;
	for (Durations::const_iterator it = m_solos.begin(); it != m_solos.end(); ++it) {
		if (time >= it->begin && time <= it->end) { m_solo = true; break; }
	}
	// Check if a long streak goal has been reached (display a nasty combo breaker popup)
	if (m_streak >= getNextBigStreak(m_bigStreak)) {
		m_bigStreak = getNextBigStreak(m_bigStreak);
		m_starmeter += streakStarBonus;
		m_listener.onBigStreak(m_bigStreak);
	}
	// During GodMode, correctness is full, no matter what
	if (m_starpower.get() > 0.01) m_correctness.setTarget(1.0, true);
}

void GuitarEngine::updateLookahead(double time) {
	bool drumfill = m_dfIt != m_drumfills.end() && m_dfIt->begin - time <= future;
	if (time != time) return;  // Check that time is not NaN
	for (auto& chord: m_chords) {
		float tBeg = static_cast<float>(chord.begin - time);
		float tEnd = static_cast<float>(m_drums ? tBeg : chord.end - time);
		if (tBeg > future) break;
		if (tEnd < past) {
			chord.passed = true; // Mark as past note for rewinding
			continue;
		}
		// Don't show past chords when rewinding
		if (chord.passed || (tBeg > maxTolerance && chord.status > 0)) continue;
		// Handle notes during drum fills / BREs
		if (drumfill && chord.begin >= m_dfIt->begin - maxTolerance
		  && chord.begin <= m_dfIt->end + maxTolerance) {
			if (chord.status == 0) {
				chord.status = chord.polyphony; // Mark as hit so that streak doesn't reset
				m_drumfillScore += static_cast<float>(chord.polyphony) * 50.0f; // Count points from notes under drum fill
			}
			continue;
		}
	}
}

/// Attempt to activate GodMode
void GuitarEngine::activateStarpower() {
	if (canActivateStarpower()) {
		m_starmeter = 0.0f;
		m_starpower.setValue(1.0f);
		m_listener.onStarpowerActivated();
	}
}

/// Mark the holding of a note as ended
void GuitarEngine::endHold(unsigned fret, double time) {
	if (fret >= m_pads || !m_holds[fret]) return;
	m_events[m_holds[fret] - 1].glow.setTarget(0.0);
	m_events[m_holds[fret] - 1].whammy.setTarget(0.0, true);
	m_holds[fret] = 0;
	if (time > 0) { // Do we set the releaseTime?
		// Search for the Chord this hold belongs to
		for (auto& chord: m_chords) {
			if (time > chord.begin + maxTolerance && time < chord.end - maxTolerance) {
				chord.releaseTimes[fret] = time;
				if (time >= chord.end - maxTolerance) chord.passed = true; // Mark as past note for rewinding
				else m_correctness.setValue(static_cast<float>(missVolumeTarget()));  // Note: if still holding some frets, proper percentage will be set in hold handling
				break;
			}
		}
	}
}

/// Do stuff when a note is played incorrectly
void GuitarEngine::fail(double time, int fret) {
	if (fret == -2) return; // Tapped note
	if (fret == -1) {
		for (unsigned i = 0; i < m_pads; ++i) endHold(i, time);
	}
	if (m_starpower.get() < 0.01f) {
		// Reduce points and play fail sample only when GodMode is deactivated
		m_events.push_back(PlayEvent(time, 0, fret));
		m_listener.onMiss(fret);
		// remove equivalent of 1 perfect hit for every note
		// kids tend to play a lot of extra notes just for the fun of it.
		// need to make sure they don't end up with a score of zero
		if (settings.missPenalty) {
			m_score -= (m_level == Difficulty::KIDS) ? points(0)/2.0f : points(0);
		}
		m_correctness.setTarget(missVolumeTarget(), true);  // Instantly fail correctness
	}
	endStreak();
}

/// Ends the Big Rock Ending and gives score if appropriate
void GuitarEngine::endBRE() {
	float l = static_cast<float>(m_dfIt->end - m_dfIt->begin);
	// Completion requires ~ 6 hits per second
	if (m_drumfillHits / l >= 6.0f) {
		m_score += m_drumfillScore; // Add score from overlapped notes if there were any
		m_score += 50.0f * l; // Add score as if it were a single long hold
	}
	m_drumfillHits = 0.0f;
	m_drumfillScore = 0.0f;
	m_dfIt = m_drumfills.end();
}

/// Calculates the start and end times for the next/current drum fill
/// Also activates GodMode if fill went well
void GuitarEngine::updateDrumFill(double time) {
	// Check if fill is over
	if (m_dfIt != m_drumfills.end()) {
		// Reset stuff for drum fills but not for BREs (handled elsewhere)
		if (time > m_dfIt->end - past && (m_dfIt != (--m_drumfills.end()) || (m_drums && !m_songHasBRE))) {
			m_drumfillHits = 0.0f;
			m_drumfillScore = 0.0f;
			m_dfIt = m_drumfills.end();
		}
		return;
	} else if (m_drums && canActivateStarpower()) {
		// Search for the next drum fill
		for (Durations::const_iterator it = m_drumfills.begin(); it != m_drumfills.end(); ++it) {
			if (it->begin >= time + future) { m_dfIt = it; return; }
		}
	} else if (!m_drums && m_drumfills.back().begin >= time + future) {
		m_dfIt = (--m_drumfills.end()); return; // Guitar Big Rock Ending
	} else if (m_drums && m_songHasBRE && m_drumfills.back().begin <= time + future) {
		m_dfIt = (--m_drumfills.end()); return; // Drum Big Rock Ending
	}
	m_dfIt = m_drumfills.end(); // Reset iterator
}

/// Handle drum hit scoring
void GuitarEngine::drumHit(double time, unsigned layer, unsigned fret) {
	SpdLogger::debug(LogSystem::INSTRUMENTS, "DrumHit={}, Layer={}, Fret={}.", time, layer, fret);
	// Handle drum fills
	if (m_dfIt != m_drumfills.end() && time >= m_dfIt->begin - maxTolerance
	  && time <= m_dfIt->end + maxTolerance) {
		m_drumfillHits += 1.0f;
		// Check if we hit the final note in drum fill to activate starpower and get the points
		if (fret == 4 && time >= m_dfIt->end - maxTolerance
		  && (!m_songHasBRE || m_dfIt != (--m_drumfills.end()))) {
			// GodMode and scores require ~ 6 hits per second
			if (m_drumfillHits >= drumfill_min_rate * (m_dfIt->end - m_dfIt->begin)) {
				activateStarpower();
				m_score += m_drumfillScore;
			}
			m_drumfillScore = 0.0f;
			m_drumfillHits = 0.0f;
		}
		m_listener.onFlame(fret);
		return;
	}
	// Find any suitable note within the tolerance
	double tolerance = maxTolerance;
	double signed_error = maxTolerance;
	auto best = m_chords.end();
	// when we get here m_chordIt points to the last best fit chord
	for (auto it = m_chordIt; it != m_chords.end() && it->begin <= time + tolerance; ++it) {
		// it->dur[static_cast<size_t>(fret)]          == NULL for a chord that doesn't include the fret played (pad hit)
		// m_notes[it->dur[static_cast<size_t>(fret)]] != 0    when the fret played (pad hit) was already played
		if (m_level == Difficulty::KIDS) {
			// in kiddy mode we don't care about the correct pad
			// all that matters is that there is still a missing note in that chord
			if (it->status == it->polyphony) continue;
		} else if (m_notes[it->dur[fret]]) continue;  // invalid fret/hit or already played

		// Check Pro Mode stuff...
		if ( m_proMode ) {
			if ( fret > 1 && it->fret_cymbal[fret] && layer != 2 ) continue; // require cymbal if it's a cymbal.
			if ( fret > 1 && !it->fret_cymbal[fret] && layer != 1 ) continue; // require not a cymbal if it's not a cymbal.
		}

		double error = std::abs(it->begin - time);
		if (error < tolerance) {
			best = it;
			tolerance = error;
			signed_error = it->begin - time;
		}
	}
	if (best == m_chords.end()) fail(time, static_cast<int>(fret));  // None found
	else {
		// Skip all chords earlier than the best fit chord
		for (; best != m_chordIt; ++m_chordIt) {
			// End streak if skipped chords had not been played properly
			if (m_chordIt->status < m_chordIt->polyphony) endStreak();
		}
		++m_chordIt->status;  // One more drum belonging to the chord hit
		double percentage = clamp(double(m_chordIt->status) / m_chordIt->polyphony); // FIXME: clamping should not be necessary but polyphony seems incorrect
		m_correctness.setTarget(1.0, true);  // Instantly correct
		m_correctness.setTarget(percentage);  // ... but keep fading if chord is incomplete
		Duration const* dur = m_chordIt->dur[fret];
		// Record the hit event
		m_events.push_back(PlayEvent(time, 1, static_cast<int>(fret), dur));
		m_notes[dur] = static_cast<unsigned>(m_events.size());
		// Scoring - be a little more generous for kids
		double score = (m_level == Difficulty::KIDS) ? points(tolerance/2.0f) : points(tolerance);
		m_chordIt->score += static_cast<float>(score);
		m_score += score;
		if (!m_drumfills.empty()) m_starmeter += static_cast<float>(score); // Only add starmeter if it's possible to activate GodMode
		// Graphical effects
		m_listener.onFlame(fret);
		if (fret == to_underlying(input::ButtonId::DRUMS_KICK)) m_listener.onDrumKick(); // Do a jump for bass drum
		// All drums of the chord hit already?
		if (m_chordIt->status == m_chordIt->polyphony) {
			//m_score -= m_chordIt->score;
			//m_chordIt->score *= m_chordIt->polyphony;
			//m_score += m_chordIt->score;
			m_streak += 1;
			if (m_streak > m_longestStreak) m_longestStreak = m_streak;
			// Handle Big Rock Ending scoring
			if (m_drumfillHits > 0 && &*best == &m_chords.back()) endBRE();  // Same chord object, not just the same frets
			// ErrorMeter
			m_listener.onErrorMeter(static_cast<float>(signed_error));
		}
	}
}

/// Handle guitar events and scoring
void GuitarEngine::guitarPlay(double time, input::Event const& ev) {
	bool picked = (ev.button == input::ButtonId::GUITAR_PICK_UP || ev.button == input::ButtonId::GUITAR_PICK_DOWN) && ev.pressed();
	unsigned buttonId = to_underlying(ev.button.id);
	// Handle Big Rock Ending
	if (m_dfIt != m_drumfills.end() && time >= m_dfIt->begin - maxTolerance
	  && time <= m_dfIt->end + maxTolerance) {
		if (!ev.pressed() || buttonId >= m_pads) return;  // No processing for release events or non-fret buttons
		m_drumfillHits += 1;
		m_listener.onFlame(buttonId);
		return;
	}
	bool frets[max_panels];  // The combination about to be played
	if (picked) {
		for (unsigned fret = 0; fret < m_pads; ++fret) {
			frets[fret] = m_pressed[fret + 1u];
		}
	} else { // Attempt to tap
		if (buttonId >= m_pads) return;
		if (m_correctness.get() < 0.5f && m_starpower.get() < 0.001f) return; // Hammering not possible at the moment
		for (unsigned fret = 0; fret < m_pads; ++fret) frets[fret] = false;
		// Note: frets[] was just cleared, held buttons are in m_pressed (fret f is m_pressed[f + 1])
		for (unsigned fret = buttonId + 1; fret < m_pads; ++fret) {
			if (m_pressed[fret + 1]) return; // Extra buttons on right side
		}
		if (ev.pressed()) {
			// Hammer-on, the fret pressed is played
			frets[buttonId] = true;
		} else {
			// Pull off, find the note to played that way
			int fret = static_cast<int>(buttonId);
			do {
				if (--fret < 0) return; // No frets pressed -> not a pull off
			} while (!m_pressed[fret + 1]);
			frets[fret] = true;
		}
	}
	// Find any suitable note within the tolerance
	double tolerance = maxTolerance;
	double signed_error = maxTolerance;
	auto best = m_chords.end();
	for (auto it = m_chordIt; it != m_chords.end() && it->begin <= time + tolerance; ++it) {
		if (it->status > 1) continue; // Already picked, can't play again
		if (!picked) { // Tapping rules
			if (it->status > 0) continue; // Already tapped, can't tap again
			if (!it->tappable) continue; // Cannot tap
			auto tmp = it;
			if ( (tmp == m_chords.begin() || (--tmp)->status == 0) &&
			  m_starpower.get() < 0.001f) continue; // The previous note not played
		}
		if (!it->matches(frets)) continue;
		double error = std::abs(it->begin - time);
		if (error < tolerance) {
			best = it;
			tolerance = error;
			signed_error = it->begin - time;
		}
	}
	if (best == m_chords.end()) fail(time, picked ? -1 : -2);
	else { // Correctly hit
		m_chordIt = best;
		float& score = m_chordIt->score;
		m_score -= score;
		m_starmeter -= score;
		bool first_time = (score == 0 ? true : false);
		if (first_time) m_streak++;
		if (m_streak > m_longestStreak) m_longestStreak = m_streak;
		score = static_cast<float>(points(tolerance));
		score *= static_cast<float>(m_chordIt->polyphony);
		m_chordIt->status = 1 + picked;
		m_score += score;
		m_starmeter += score;
		m_correctness.setTarget(1.0, true); // Instantly go to one
		m_listener.onErrorMeter(static_cast<float>(signed_error));
		for (unsigned fret = 0; fret < m_pads; ++fret) {
			if (!m_chordIt->fret[fret]) continue;
			Duration const* dur = m_chordIt->dur[fret];
			m_events.push_back(PlayEvent(time, 1 + picked, static_cast<int>(fret), dur));
			m_notes[dur] = static_cast<unsigned>(m_events.size());
			m_holds[fret] = static_cast<unsigned>(m_events.size());
			if (first_time) m_listener.onFlame(fret);
		}
		// Handle Big Rock Ending scoring
		if (m_drumfillHits > 0 && &*best == &m_chords.back()) endBRE();  // Same chord object, not just the same frets
	}
}

bool GuitarEngine::updateTom(unsigned int tomTrack, int fretId) {
	auto tomTrackIt = m_track->nm.find(tomTrack);
	if (tomTrackIt == m_track->nm.end()) return false;  // Track not found
	auto chordIt = m_chords.begin();
	for (Duration const& tom: tomTrackIt->second) {
		// Iterate over chords of the song
		for (; chordIt != m_chords.end() && chordIt->begin < tom.end; ++chordIt) {
			if (!chordIt->fret[fretId]) continue;  // Chord doesn't contain the fret we are looking for
			chordIt->fret_cymbal[fretId] = (chordIt->begin < tom.begin);  // If not within tom, it is a cymbal
		}
	}
	return true;
}

/// Create the Chord structures for the current track/difficulty level
void GuitarEngine::updateChords() {
	m_chords.clear();
	m_solos.clear();
	m_drumfills.clear();
	m_scoreFactor = 0;
	NoteMap const& nm = m_track->nm;

	Durations::size_type pos[5] = {}, size[5] = {};
	Durations const* durations[5] = {};
	for (unsigned fret = 0; fret < m_pads; ++fret) {
		unsigned basepitch = basePitch(m_level);
		auto it = nm.find(basepitch + fret);
		if (it == nm.end()) continue;
		durations[fret] = &it->second;
		size[fret] = durations[fret]->size();
	}
	double lastEnd = 0.0;
	const double tapMaxDelay = 0.15;  // Delay from the end of the previous note
	while (true) {
		// Find the earliest
		double t = getInf();
		for (unsigned fret = 0; fret < m_pads; ++fret) {
			if (pos[fret] == size[fret]) continue;
			Durations const& dur = *durations[fret];
			t = std::min(t, dur[pos[fret]].begin);
		}
		// Quit processing if none were left
		if (t == getInf()) break;
		// Construct a chord
		GuitarChord c;
		c.begin = t;
		unsigned tapfret = 0;
		for (unsigned fret = 0; fret < m_pads; ++fret) {
			if (pos[fret] == size[fret]) continue;
			Durations const& dur = *durations[fret];
			Duration const& d = dur[pos[fret]];
			if (d.begin > t) continue;
			c.end = std::max(c.end, d.end);
			c.fret[fret] = true;
			c.dur[fret] = &d;
			tapfret = fret;
			++c.polyphony;
			++pos[fret];
			m_scoreFactor += 50.0f;
			if (d.end - d.begin > 0.0f) m_scoreFactor += 50.0f * (d.end - d.begin);
		}
		// Check if the chord is tappable
		if (!m_drums && c.polyphony == 1) {
			c.tappable = true;
			if (m_chords.empty() || m_chords.back().fret[tapfret]) c.tappable = false;
			if (lastEnd + tapMaxDelay < t) c.tappable = false;
		}
		lastEnd = c.end;
		m_chords.push_back(c);
	}
	m_chordIt = m_chords.begin();

	m_hasTomTrack = false;
	if(m_drums) {
		// HiHat/Rack Tom 1 detection
		m_hasTomTrack = updateTom(110, to_underlying(input::ButtonId::DRUMS_YELLOW)) || m_hasTomTrack;
		// Ride Cymbal/Rack Tom 2 detection
		m_hasTomTrack = updateTom(111, to_underlying(input::ButtonId::DRUMS_BLUE)) || m_hasTomTrack;
		// Crash Cymbal/Floor Tom detection
		m_hasTomTrack = updateTom(112, to_underlying(input::ButtonId::DRUMS_GREEN)) || m_hasTomTrack;
	}

	// Solos
	auto solotrack = nm.find(103); // 103 = Expert Solo - used for every difficulty
	if (solotrack != nm.end()) {
		for (auto const& solo: solotrack->second) {
			// Require at least 6s length in order to avoid starpower sections
			if (solo.end - solo.begin >= 6.0f) m_solos.push_back(solo);
		}
	}
	// Drum fills
	auto dfTrack = nm.find(124); // 124 = drum fills (actually 120-124, but one is enough)
	if (dfTrack != nm.end()) {
		m_drumfills = dfTrack->second;
		// Big Rock Ending scoring (single hold note)
		if (!m_drums || m_songHasBRE)
			m_scoreFactor += 50.0f * (m_drumfills.back().end - m_drumfills.back().begin);
	}
	m_dfIt = m_drumfills.end();

	// Normalize maximum score factor
	m_scoreFactor = 10000.0f / m_scoreFactor;
}
