#include "guitargraph.hh"

#include "fs.hh"
#include "log.hh"
#include "song.hh"
#include "i18n.hh"
#include "graphic/view_trans.hh"

#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <fmt/format.h>

namespace {
	#if 0 // Here is some dummy gettext calls to populate the dictionary
	_("Kids") _("Easy") _("Medium") _("Hard") _("Expert")
	#endif
	struct Diff { std::string name; unsigned basepitch; } diffv[] = {
		{ "Kids", 0x3C },
		{ "Easy", 0x3C },
		{ "Medium", 0x48 },
		{ "Hard", 0x54 },
		{ "Expert", 0x60 }
	};
	const float g_angle = 1.4f; // How many radians to rotate the fretboards
	const float past = -0.2f; // Relative time from cursor that is considered past (out of screen)
	const float future = 1.5f; // Relative time from cursor that is considered future (out of screen)
	const float timescale = 25.0f; // Multiplier to get graphics units from time
	const float texCoordStep = -0.5f; // Two beat lines per neck texture => 0.5 tex units per beat
	const float flameSpd = 6.0f; // Multiplier for flame growth
	// Note: t is difference from playback time so it must be in range [past, future]
	float time2y(float t) { return -timescale * (t - past) / (future - past); }
	float time2a(float t) {
		float a = static_cast<float>(clamp(1.0 - t / future)); // Note: we want 1.0 alpha already at zero t.
		return std::pow(a, 0.8f); // Nicer curve
	}
	float y2a(float y) { return time2a(past - y / timescale * (future - past)); }
	float tc(float y) { return static_cast<float>(y * 0.1); } // Get texture coordinates for animating hold notes
	const double maxTolerance = 0.15; // Maximum error in seconds

	const float drumfill_min_rate = 6.0; // The rate of hits per second required to complete a drum fill
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
	inline float blend(float a, float b, float f) { return a*f + b*(1.0f-f); }
}

void GuitarGraph::initGuitar() {
	// Copy all tracks of guitar types (not DRUMS and not KEYBOARD) to m_instrumentTracks
	for (auto const& elem: m_song.instrumentTracks) {
		std::string index = elem.first;
		if (index != TrackName::DRUMS &&
			index != TrackName::DRUMS_SNARE &&
			index != TrackName::DRUMS_CYMBALS &&
			index != TrackName::DRUMS_TOMS &&
			index != TrackName::KEYBOARD)  {
				m_instrumentTracks[index] = &elem.second;
			}
	}
	if (m_instrumentTracks.empty()) throw std::logic_error("No guitar tracks found");

	// Adding fail samples
	m_samples.push_back("guitar fail1");
	m_samples.push_back("guitar fail2");
	m_samples.push_back("guitar fail3");
	m_samples.push_back("guitar fail4");
	m_samples.push_back("guitar fail5");
	m_samples.push_back("guitar fail6");
}

void GuitarGraph::initDrums() {
	// Copy all tracks of drum type to m_instrumentTracks
	for (auto const& elem: m_song.instrumentTracks) {
		std::string index = elem.first;
		if (index == TrackName::DRUMS ||
			index == TrackName::DRUMS_SNARE ||
			index == TrackName::DRUMS_CYMBALS ||
			index == TrackName::DRUMS_TOMS ) {
				m_instrumentTracks[index] = &elem.second;
			}
	}
	if (m_instrumentTracks.empty()) throw std::logic_error("No drum tracks found");

	// Adding fail samples
	m_samples.push_back("drum bass");
	m_samples.push_back("drum snare");
	m_samples.push_back("drum hi-hat");
	m_samples.push_back("drum tom1");
	m_samples.push_back("drum cymbal");
	//m_samples.push_back("drum tom2");
}

GuitarGraph::GuitarGraph(Game &game, Audio& audio, Song const& song, input::DevicePtr dev, int number):
  InstrumentGraph(game, audio, song, dev),
  m_tail(findFile("tail.svg")),
  m_tail_glow(findFile("tail_glow.svg")),
  m_tail_drumfill(findFile("tail_drumfill.svg")),
  m_flame(findFile("flame.svg")),
  m_flame_godmode(findFile("flame_godmode.svg")),
  m_tap(findFile("tap.svg")),
  m_neckglow(findFile("neck_glow.svg")),
  m_neckglowColor(),
  m_drums(dev->type == input::DevType::DRUMS),
  m_level(),
  m_track_index(m_instrumentTracks.end()),
  m_dfIt(m_drumfills.end()),
  m_errorMeter(0.0, 2.0),
  m_errorMeterFlash(0.0, 4.0),
  m_errorMeterFade(0.0, 0.333),
  m_drumJump(0.0, 12.0),
  m_starpower(0.0, 0.1),
  m_starmeter(),
  m_drumfillHits(),
  m_drumfillScore(),
  m_soloTotal(),
  m_soloScore(),
  m_solo(),
  m_hasTomTrack(false),
  m_proMode(config["game/drum_promode"].b()),
  m_whammy(0)
{
	if(m_drums) {
		initDrums();
	} else {
		initGuitar();
	}
	// Load 3D fret objects
	m_fretObj.load(findFile("fret.obj"));
	m_tappableObj.load(findFile("fret_tap.obj"));
	// Score calculator (TODO a better one)
	m_scoreText = std::make_unique<SvgTxtThemeSimple>(findFile("sing_score_text.svg"), config["graphic/text_lod"].f());
	m_streakText = std::make_unique<SvgTxtThemeSimple>(findFile("sing_score_text.svg"), config["graphic/text_lod"].f());
	for (size_t i = 0; i < max_panels; ++i) {
		m_pressed_anim[i].setRate(5.0);
		m_holds[i] = 0;
	}
	m_pads = 5;
	m_track_index = m_instrumentTracks.begin();
	while (number--)
		if (++m_track_index == m_instrumentTracks.end()) m_track_index = m_instrumentTracks.begin();
	// Pick a nice default difficulty (note: the execution of || stops when true is returned)
	difficulty(Difficulty::EASY) ||
	difficulty(Difficulty::SUPAEASY) ||
	difficulty(Difficulty::MEDIUM) ||
	difficulty(Difficulty::AMAZING) ||
	(difficultyAuto(), true);
	updateNeck();
	setupJoinMenu();
}

void GuitarGraph::setupJoinMenuDifficulty() {
	ConfigItem::OptionList ol;
	unsigned short cur = 0;
	// Add difficulties to the option list
	auto maxLevel = to_underlying(Difficulty::COUNT);
	for (int level = 0; level < maxLevel; ++level) {
		if (difficulty(Difficulty(level), true)) {
			ol.push_back(std::to_string(level));
			if (Difficulty(level) == m_level) cur = static_cast<unsigned short>(ol.size()-1u);
		}
	}
	m_selectedDifficulty = ConfigItem(ol); // Create a ConfigItem from the option list
	m_selectedDifficulty.select(cur); // Set the selection to current level
	m_menu.add(MenuOption("", _("Select difficulty"))).changer(m_selectedDifficulty); // MenuOption that cycles the options
	m_menu.back().setDynamicName(m_difficultyOpt); // Set the title to be dynamic
}

void GuitarGraph::setupJoinMenuDrums() {
	setupJoinMenuDifficulty();
	m_menu.add(MenuOption(_("Lefty-mode"), "")).changer(m_leftymode);
	m_menu.back().setDynamicComment(m_leftyOpt);
}

void GuitarGraph::setupJoinMenuGuitar() {
	ConfigItem::OptionList ol;
	unsigned short cur = 0;
	// Add tracks to option list
	for (InstrumentTracksConstPtr::const_iterator it = m_instrumentTracks.begin(); it != m_instrumentTracks.end(); ++it) {
		ol.push_back(it->first);
		if (m_track_index->first == it->first) cur = static_cast<unsigned short>(ol.size() - 1); // Find the index of current track
	}
	m_selectedTrack = ConfigItem(ol); // Create a ConfigItem from the option list
	m_selectedTrack.select(cur); // Set the selection to current track
	m_menu.add(MenuOption("", _("Select track"))).changer(m_selectedTrack); // MenuOption that cycles the options
	m_menu.back().setDynamicName(m_trackOpt); // Set the title to be dynamic
	setupJoinMenuDifficulty();
	m_menu.add(MenuOption(_("Lefty-mode"), "")).changer(m_leftymode);
	m_menu.back().setDynamicComment(m_leftyOpt);
}

void GuitarGraph::setupJoinMenu() {
	m_menu.clear();
	updateJoinMenu();
	// Populate root menu
	m_menu.add(MenuOption(_("Ready!"), _("Start performing!")));
	if(m_drums) {
		setupJoinMenuDrums();
	} else {
		setupJoinMenuGuitar();
	}
	m_menu.add(MenuOption(_("Quit"), _("Exit to song browser"))).screen("Songs");
}

void GuitarGraph::updateJoinMenu() {
	m_trackOpt = getTrack();
	m_difficultyOpt =  getDifficultyString();
	std::string s("\n (");
	std::string le = m_leftymode.b() ? _("ON") : _("OFF");
	m_leftyOpt = _("Toggle lefty-mode") + s + le + ")";
}

/// Load the appropriate neck texture
void GuitarGraph::updateNeck() {
	// TODO: Optimize with texture cache
	std::string index = m_track_index->first;
	if (index == TrackName::DRUMS) m_neck = std::make_unique<Texture>(findFile("drumneck.svg"));
	else if (index == TrackName::KEYBOARD) m_neck = std::make_unique<Texture>(findFile("guitarneck.svg"));
	else if (index == TrackName::BASS) m_neck = std::make_unique<Texture>(findFile("bassneck.svg"));
	else m_neck = std::make_unique<Texture>(findFile("guitarneck.svg"));
}

/// Cycle through the different tracks
void GuitarGraph::changeTrack(int dir) {
	if (dir >= 0) ++m_track_index; else --m_track_index;
	if (m_track_index == m_instrumentTracks.end()) m_track_index = m_instrumentTracks.begin();
	else if (m_track_index == (--m_instrumentTracks.end())) m_track_index = (--m_instrumentTracks.end());
	difficultyAuto(true);
	updateNeck();
	setupJoinMenu();  // Reset menu as difficulties might have changed
	m_menu.select(1); // Restore selection to the track item
}

/// Set specific track
void GuitarGraph::setTrack(const std::string& track) {
	InstrumentTracksConstPtr::const_iterator it = m_instrumentTracks.find(track);
	if (it != m_instrumentTracks.end()) m_track_index = it;
	difficultyAuto(true);
	updateNeck();
	setupJoinMenu();  // Reset menu as difficulties might have changed
	m_menu.select(1); // Restore selection to the track item
}

/// Get the trackname string
std::string GuitarGraph::getTrack() const {
	return _(m_track_index->first.c_str());
}

/// Get the difficulty as displayable string
std::string GuitarGraph::getDifficultyString() const {
	return _(diffv[to_underlying(m_level)].name.c_str());
}

/// Get a string id for track and difficulty
std::string GuitarGraph::getModeId() const {
	return m_track_index->first + " - " + diffv[to_underlying(m_level)].name;
}

/// Cycle through difficulties
void GuitarGraph::changeDifficulty(int dir) {
	auto maxLevel = to_underlying(Difficulty::COUNT);
	auto currentLevel = to_underlying(m_level);
	for (int level = ((int)m_level + dir) % maxLevel; level != currentLevel;
	  level = (level+dir) % maxLevel)
		if (difficulty(Difficulty(level))) return;
}

/// Find an initial difficulty level to use
void GuitarGraph::difficultyAuto(bool tryKeep) {
	if (tryKeep && difficulty(Difficulty(m_level))) return;
	auto maxLevel = to_underlying(Difficulty::COUNT);
	for (int level = 0; level < maxLevel; ++level) if (difficulty(Difficulty(level))) return;
	throw std::runtime_error("No difficulty levels found for track " + m_track_index->first);
}

/// Attempt to use a given difficulty level
bool GuitarGraph::difficulty(Difficulty level, bool check_only) {
	InstrumentTrack const& track = *m_track_index->second;
	// Find the stream number
	for (auto const& elem: m_song.instrumentTracks) {
		if (&track == &elem.second) break;
	}
	// Check if the difficulty level is available
	unsigned basepitch = diffv[to_underlying(level)].basepitch;
	NoteMap const& nm = track.nm;
	unsigned fail = 0;
	for (unsigned fret = 0; fret < m_pads; ++fret) if (nm.find(basepitch + fret) == nm.end()) ++fail;
	if (fail == m_pads) return false;
	if (check_only) return true;
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

/// Core engine
void GuitarGraph::engine() {
	double time = m_audio.getPosition();
	time -= config["audio/controller_delay"].f();
	doUpdates();
	if (!m_drumfills.empty()) updateDrumFill(time); // Drum Fills / BREs
	m_whammy = 0;
	// Countdown to start
	handleCountdown(time, time < getNotesBeginTime() ? getNotesBeginTime() : m_jointime+1);
	// Handle all events
	for (input::Event ev; m_dev->getEvent(ev); ) {
		unsigned buttonId = to_underlying(ev.button.id);
		// Lefty mode flip of buttons
		if (m_leftymode.b() && m_drums && ev.source.type != input::SourceType::MIDI) {
			unsigned layer = ev.button.layer(), num = ev.button.num();
			// Layers 0-1: reverse all but kick; layer 2: swap yellow and blue cymbals
			if ((layer < 2 && num >= 1 && num <= 4) || (layer == 2 && num >= 2 && num <= 3)) ev.button = input::Button(layer, 5 - num);
		}
		// Keypress anims
		{
			unsigned layer = ev.button.layer(), num = ev.button.num();
			// Guitar numbering hack
			if (!m_drums) {
				if (ev.button == input::ButtonId::GUITAR_PICK_DOWN || ev.button == input::ButtonId::GUITAR_PICK_UP) { layer = 0; num = 0; }
				else ++num;
			}
			// Update key pressed status
			if (layer < 8 && num < max_panels) {
				if (!m_drums && num > 0) m_pressed[num] = static_cast<bool>(ev.value); // Only guitar frets can be held
				if (ev.pressed()) m_pressed_anim[num].setValue(1.0); // Hit flash
			}
		}
		m_dead = 0; // Keep alive
		// Menu keys
		if (menuOpen()) {
			// Check first regular keys
			if (ev.pressed()) {
				if (ev.nav == input::NavButton::START) m_menu.action(m_game);
				else if (ev.nav == input::NavButton::LEFT) m_menu.action(m_game, -1);
				else if (ev.nav == input::NavButton::UP) m_menu.move(-1);
				else if (ev.nav == input::NavButton::DOWN) m_menu.move(1);
				else if (ev.nav == input::NavButton::RIGHT) m_menu.action(m_game, 1);
				else if (ev.nav == input::NavButton::CANCEL) m_menu.close();
				if (!m_drums) {
					if (ev.button == input::ButtonId::GUITAR_GREEN) m_menu.action(m_game, -1);
					else if (ev.button == input::ButtonId::GUITAR_RED) m_menu.action(m_game, 1);
				}
			}
			// See if anything changed
			if (!m_drums && m_selectedTrack.so() != m_track_index->first) setTrack(m_selectedTrack.so());
			else if (std::stoi(m_selectedDifficulty.so()) != to_underlying(m_level))
				difficulty(Difficulty(std::stoi(m_selectedDifficulty.so())));
			else if (m_rejoin.b()) { unjoin(); setupJoinMenu(); m_dev->pushEvent(input::Event()); /* FIXME: HACK! */ }
			// Sync menu items & captions
			updateJoinMenu();
			break;

		// If the songs hasn't yet started, we want key presses to bring join menu back (not pause menu)
		} else if (time < -2 && ev.pressed() && ev.button != input::ButtonId::GUITAR_WHAMMY && ev.button != input::ButtonId::GUITAR_GODMODE) {
			setupJoinMenu();
			m_menu.open();
			break;
		// Handle Start/Select keypresses
		} else if (!isKeyboard()) {
			if (ev.button == input::ButtonId::GENERIC_CANCEL) ev.button = input::ButtonId::GUITAR_GODMODE; // Select = GodMode
			if (ev.button == input::ButtonId::GENERIC_START) { m_menu.open(); continue; }
		}

		// Disable gameplay when game is paused
		if (m_audio.isPaused()) continue;

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
	// Handle fret hold markers
	if (!m_drums) for (unsigned num = 0; num < max_panels; ++num) m_pressed_anim[num].setTarget(m_pressed[num] ? 1.0 : 0.0);

	// Skip missed chords
	// - Only after we are so much past them that they can no longer be played (maxTolerance)
	// - For chords played or skipped by playing (i.e. play another chord that quickly follows), ++m_chordIt is done elsewhere
	while (m_chordIt != m_chords.end() && m_chordIt->begin + maxTolerance < time) {
		if (m_chordIt->status < m_chordIt->polyphony) endStreak();
		// Calculate solo total score
		if (m_solo) { m_soloScore += m_chordIt->score; m_soloTotal += static_cast<float>(m_chordIt->polyphony) * static_cast<float>(points(0));
		// Solo just ended?
		} else if (m_soloTotal > 0) {
			m_popups.push_back(Popup(std::to_string(unsigned(m_soloScore / m_soloTotal * 100)) + " %",
			  Color(0.0f, 0.8f, 0.0f), 1.0f, m_popupText.get()));
			m_soloScore = 0;
			m_soloTotal = 0;
		}
		if (!joining(time)) ++m_dead;  // Increment dead counter (but not while joining)
		++m_chordIt;
	}
	// Start decreasing correctness instantly if the current note is being played late (don't wait until maxTolerance)
	if (m_chordIt != m_chords.end() && m_chordIt->begin < time && m_chordIt->status == 0) m_correctness.setTarget(0.0);
	// Process holds
	if (!m_drums) {
		// FIXME: Why do we have per-fret hold handling, why not just as a part of the current chord?
		// FIXME: Use polyphony from proper chord (which isn't always m_chordIt); now it only counts frets currently held
		unsigned count = 0;
		bool holds = false;
		for (unsigned fret = 0; fret < m_pads; ++fret) {
			if (!m_holds[fret]) continue;
			holds = true;
			Event& ev = m_events[m_holds[fret] - 1];
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
			bool early = time - ev.dur->begin < 1.5;  // At the beginning we don't require whammy
			bool whammy = ev.whammy.get() > 0.01f;
			bool godmode = m_starpower.get() > 0.01f;
			if (early || whammy || godmode) ++count;
			// Score for holding
			m_score += t * 50.0f * m_correctness.get();
			// Whammy fills starmeter much faster
			m_starmeter += static_cast<float>(t * 50.0f * ( (ev.whammy.get() > 0.01f) ? 2.0f : 1.0f ));
			ev.holdTime = time;
			// If end reached, handle it
			if (last == ev.dur->end) endHold(fret, time);
		}
		// Set correctness as a percentage of chord being held
		if (holds) m_correctness.setTarget(clamp(double(count) / m_chordIt->polyphony));  // FIXME: polyphony doesn't seem to be set correctly for guitar tracks (workaround by clamping)
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
		m_popups.push_back(Popup(std::to_string(unsigned(m_bigStreak)) + "\n" + _("Streak!"),
		  Color(1.0f, 0.0f, 0.0f), 1.0f, m_popupText.get()));
	}
	// During GodMode, correctness is full, no matter what
	if (m_starpower.get() > 0.01) m_correctness.setTarget(1.0, true);
}

/// Attempt to activate GodMode
void GuitarGraph::activateStarpower() {
	if (canActivateStarpower()) {
		m_starmeter = 0.0f;
		m_starpower.setValue(1.0f);
		m_popups.push_back(Popup(_("God Mode\nActivated!"),
		  Color(0.3f, 0.0f, 1.0f), 0.666f, m_popupText.get(), _("Mistakes ignored!"), &m_text));
	}
}

/// New hit for the error indicator
void GuitarGraph::errorMeter(double error) {
	error /=  maxTolerance;
	m_errorMeter.setTarget(error, true);
	m_errorMeterFade.setValue(1.0f);
	if (std::abs(error) < 0.10f) m_errorMeterFlash.setValue(1.0f);
}

/// Mark the holding of a note as ended
void GuitarGraph::endHold(unsigned fret, double time) {
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
				else m_correctness.setValue(0.0f);  // Note: if still holding some frets, proper percentage will be set in hold handling
				break;
			}
		}
	}
}

/// Do stuff when a note is played incorrectly
void GuitarGraph::fail(double time, int fret) {
	if (fret == -2) return; // Tapped note
	if (fret == -1) {
		for (unsigned i = 0; i < m_pads; ++i) endHold(i, time);
	}
	if (m_starpower.get() < 0.01f) {
		// Reduce points and play fail sample only when GodMode is deactivated
		m_events.push_back(Event(time, 0, fret));
		if (fret < 0) fret = static_cast<int>(std::rand());
		m_audio.playSample(m_samples[unsigned(fret) % m_samples.size()]);
		// remove equivalent of 1 perfect hit for every note
		// kids tend to play a lot of extra notes just for the fun of it.
		// need to make sure they don't end up with a score of zero
		m_score -= (m_level == Difficulty::KIDS) ? points(0)/2.0f : points(0);
		m_correctness.setTarget(0.0, true);  // Instantly fail correctness
	}
	endStreak();
}

/// Ends the Big Rock Ending and gives score if appropriate
void GuitarGraph::endBRE() {
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
void GuitarGraph::updateDrumFill(double time) {
	// Check if fill is over
	if (m_dfIt != m_drumfills.end()) {
		// Reset stuff for drum fills but not for BREs (handled elsewhere)
		if (time > m_dfIt->end - past && (m_dfIt != (--m_drumfills.end()) || (m_drums && !m_song.hasBRE))) {
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
	} else if (m_drums && m_song.hasBRE && m_drumfills.back().begin <= time + future) {
		m_dfIt = (--m_drumfills.end()); return; // Drum Big Rock Ending
	}
	m_dfIt = m_drumfills.end(); // Reset iterator
}

/// Handle drum hit scoring
void GuitarGraph::drumHit(double time, unsigned layer, unsigned fret) {
	SpdLogger::debug(LogSystem::INSTRUMENTS, "DrumHit={}, Layer={}, Fret={}.", time, layer, fret);
	// Handle drum fills
	if (m_dfIt != m_drumfills.end() && time >= m_dfIt->begin - maxTolerance
	  && time <= m_dfIt->end + maxTolerance) {
		m_drumfillHits += 1.0f;
		// Check if we hit the final note in drum fill to activate starpower and get the points
		if (fret == 4 && time >= m_dfIt->end - maxTolerance
		  && (!m_song.hasBRE || m_dfIt != (--m_drumfills.end()))) {
			// GodMode and scores require ~ 6 hits per second
			if (m_drumfillHits >= drumfill_min_rate * (m_dfIt->end - m_dfIt->begin)) {
				activateStarpower();
				m_score += m_drumfillScore;
			}
			m_drumfillScore = 0.0f;
			m_drumfillHits = 0.0f;
		}
		m_flames[fret].push_back(AnimValue(0.0, flameSpd));
		m_flames[fret].back().setTarget(1.0);
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
			if (m_chordIt->status == m_chordIt->polyphony) continue;
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
		m_events.push_back(Event(time, 1, static_cast<int>(fret), dur));
		m_notes[dur] = static_cast<unsigned>(m_events.size());
		// Scoring - be a little more generous for kids
		double score = (m_level == Difficulty::KIDS) ? points(tolerance/2.0f) : points(tolerance);
		m_chordIt->score += static_cast<float>(score);
		m_score += score;
		if (!m_drumfills.empty()) m_starmeter += static_cast<float>(score); // Only add starmeter if it's possible to activate GodMode
		// Graphical effects
		m_flames[fret].push_back(AnimValue(0.0, flameSpd));
		m_flames[fret].back().setTarget(1.0);
		if (fret == to_underlying(input::ButtonId::DRUMS_KICK)) m_drumJump.setTarget(1.0); // Do a jump for bass drum
		// All drums of the chord hit already?
		if (m_chordIt->status == m_chordIt->polyphony) {
			//m_score -= m_chordIt->score;
			//m_chordIt->score *= m_chordIt->polyphony;
			//m_score += m_chordIt->score;
			m_streak += 1;
			if (m_streak > m_longestStreak) m_longestStreak = m_streak;
			// Handle Big Rock Ending scoring
			if (m_drumfillHits > 0 && *best == m_chords.back()) endBRE();
			// ErrorMeter
			errorMeter(static_cast<float>(signed_error));
		}
	}
}

/// Handle guitar events and scoring
void GuitarGraph::guitarPlay(double time, input::Event const& ev) {
	bool picked = (ev.button == input::ButtonId::GUITAR_PICK_UP || ev.button == input::ButtonId::GUITAR_PICK_DOWN) && ev.pressed();
	unsigned buttonId = to_underlying(ev.button.id);
	// Handle Big Rock Ending
	if (m_dfIt != m_drumfills.end() && time >= m_dfIt->begin - maxTolerance
	  && time <= m_dfIt->end + maxTolerance) {
		if (!ev.pressed() || buttonId >= m_pads) return;  // No processing for release events or non-fret buttons
		m_drumfillHits += 1;
		m_flames[buttonId].push_back(AnimValue(0.0, flameSpd));
		m_flames[buttonId].back().setTarget(1.0);
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
		for (unsigned fret = buttonId + 1; fret < m_pads; ++fret) {
			if (frets[fret]) return; // Extra buttons on right side
		}
		if (ev.pressed()) {
			// Hammer-on, the fret pressed is played
			frets[buttonId] = true;
		} else {
			// Pull off, find the note to played that way
			int fret = static_cast<int>(buttonId);
			do {
				if (--fret < 0) return; // No frets pressed -> not a pull off
			} while (!frets[fret]);
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
		errorMeter(static_cast<float>(signed_error));
		for (unsigned fret = 0; fret < m_pads; ++fret) {
			if (!m_chordIt->fret[fret]) continue;
			Duration const* dur = m_chordIt->dur[fret];
			m_events.push_back(Event(time, 1 + picked, static_cast<int>(fret), dur));
			m_notes[dur] = static_cast<unsigned>(m_events.size());
			m_holds[fret] = static_cast<unsigned>(m_events.size());
			if (first_time) {
				m_flames[fret].push_back(AnimValue(0.0, flameSpd));
				m_flames[fret].back().setTarget(1.0);
			}
		}
		// Handle Big Rock Ending scoring
		if (m_drumfillHits > 0 && *best == m_chords.back()) endBRE();
	}
}

/// Modify color based on things like GodMode and solos
Color const GuitarGraph::colorize(Color c, double time) const {
	const static Color godmodeC(0.5f, 0.5f, 1.0f); // Color for full GodMode
	const static Color soloC(0.2f, 0.9f, 0.2f); // Color for solo notes
	// Solo? (cannot use m_solo, as time can be in future)
	for (auto const& solo: m_solos) {
		if (time >= solo.begin && time <= solo.end) { c = soloC; break; }
	}
	float f = static_cast<float>(m_starpower.get());
	if (f < 0.001f) return c;
	f = std::sqrt(std::sqrt(f));
	c.r = blend(godmodeC.r, c.r, f);
	c.g = blend(godmodeC.g, c.g, f);
	c.b = blend(godmodeC.b, c.b, f);
	return c;
}

namespace {
	const float fretWid = 0.5f; // The actual width is two times this

	/// Create a symmetric vertex pair of given data
	void vertexPair(glutil::VertexArray& va, float x, float y, Color color, float ty, float fretW = fretWid, float zn = 0.0f) {
		color.a = y2a(y);
		{
			glmath::vec4 c(color.r, color.g, color.b, color.a);
			va.color(c).texCoord(0.0f, ty).vertex(x - fretW, y, 0.1f + zn);
			va.color(c).texCoord(1.0f, ty).vertex(x + fretW, y, 0.1f - zn);
		}
	}

}

void GuitarGraph::drawNotes(double time) {
	glutil::UseDepthTest depthtest;
	// Draw drum fills / Big Rock Endings
	bool drumfill = m_dfIt != m_drumfills.end() && m_dfIt->begin - time <= future;
	if (drumfill) {
		drawDrumfill(m_dfIt->begin - time, m_dfIt->end - time);
		// If it is a drum fill (not BRE), draw the final note
		if (m_drums && (!m_song.hasBRE || (m_dfIt != (--m_drumfills.end())))) {
			Color c(0.7f, 0.7f, 0.7f);
			if (m_drumfillHits >= drumfill_min_rate * (m_dfIt->end - m_dfIt->begin))
				c = colorize(color(4), m_dfIt->end);
			drawNote(4, c, m_dfIt->end - time, m_dfIt->end - time, 0.0f, false, false, 0.0, 0.0);
		}
	}
	if (time != time) return;  // Check that time is not NaN

	glmath::vec4 neckglow{};  // Used for calculating the average neck color

	// Iterate chords
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
		// Loop through the frets
		for (unsigned fret = 0; fret < m_pads; ++fret) {
			if (!chord.fret[fret] || (tBeg > maxTolerance && chord.releaseTimes[fret] > 0)) continue;
			if (tEnd > future) tEnd = future;
			unsigned event = m_notes[chord.dur[fret]];
			float glow = 0.0f;
			float whammy = 0.0f;
			if (event > 0) {
				glow = static_cast<float>(m_events[event - 1].glow.get());
				whammy = static_cast<float>(m_events[event - 1].whammy.get());
			}
			// Set the default color (disabled state)
			Color c(0.5f, 0.5f, 0.5f);
			if (!joining(time)) {
				// Get a color for the fret and adjust it if GodMode is on
				c = colorize(color(fret), chord.begin);
				if (glow > 0.1f) { neckglow = neckglow + glmath::vec4(c.r, c.g, c.b, 1.0f); } // neck glow tracking
				// Further adjust the color if the note is hit
				c.r += glow * 0.2f;
				c.g += glow * 0.2f;
				c.b += glow * 0.2f;
			}
			if (glow > 0.5f && tEnd < 0.1f && chord.hitAnim[fret].get() == 0.0) chord.hitAnim[fret].setTarget(1.0);
			// Call the actual note drawing function
			bool tap = m_drums ? chord.fret_cymbal[fret] : chord.tappable;
			drawNote(fret, c, tBeg, tEnd, whammy, tap, glow > 0.5f, chord.hitAnim[fret].get(),
			  chord.releaseTimes[fret] > 0.0 ? chord.releaseTimes[fret] - time : getNaN());
		}
	}
	// Mangle neck glow color as needed
	// Convert sum into average and apply correctness as premultiplied alpha
	if (neckglow.w > 0.0f) neckglow = static_cast<float>((correctness() / neckglow.w)) * neckglow;
	// Blend into use slowly
	m_neckglowColor = glmath::mix(m_neckglowColor, neckglow, 0.05f);
}

float GuitarGraph::neckWidth() const {
	return static_cast<float>(std::min(0.5, m_width.get()));
}

void GuitarGraph::drawNeckStuff(double time) {
	using namespace glmath;
	mat4 m = translate(vec3(0.0f, 0.5f * virtH(), 0.0f)) * rotate(g_angle, vec3(1.0f, 0.0f, 0.0f)) * scale(neckWidth() / 5.0f);
	// Do some jumping for drums
	if (m_drums) {
		float jumpanim = static_cast<float>(m_drumJump.get());
		if (jumpanim == 1.0f) m_drumJump.setTarget(0.0f);
		if (jumpanim > 0.0f) m = translate(vec3(0.0f, -m_drumJump.get() * 0.01f, 0.0f)) * m;
	}

	auto& window = m_game.getWindow();
	//Transform trans(m);
	ViewTrans trans(window, m);

	// Draw the neck
	{
		UseTexture tex(m_game.getWindow(), *m_neck);
		glutil::VertexArray va;
		float w = (m_drums ? 2.0f : 2.5f);
		float texCoord = 0.0f;
		float tBeg = 0.0f, tEnd;
		for (auto it = m_song.beats.begin(); it != m_song.beats.end() && tBeg < future; ++it, texCoord += texCoordStep, tBeg = tEnd) {
			tEnd = static_cast<float>(*it - time);
			//if (tEnd < past) continue;
			if (tEnd > future) {
				// Crop the end off
				texCoord -= texCoordStep * (tEnd - future) / (tEnd - tBeg);
				tEnd = future;
			}
			glmath::vec4 c = colorize(Color::alpha(time2a(tEnd)), time + tBeg).linear();
			va.normal(0.0f, 1.0f, 0.0f).color(c).texCoord(0.0f, texCoord).vertex(-w, time2y(tEnd));
			va.normal(0.0f, 1.0f, 0.0f).color(c).texCoord(1.0f, texCoord).vertex(w, time2y(tEnd));
		}
		va.draw();
	}

	if (!menuOpen()) {
		// Draw the cursor
		{
			float level = static_cast<float>(m_pressed_anim[0].get());
			ColorTrans c(window, Color(level, level, level));
			drawBar(0.0f, 0.01f);
		}
		// Fret buttons on cursor
		for (unsigned fret = m_drums; fret < m_pads; ++fret) {
			float x = getFretX(fret);
			float l = static_cast<float>(m_pressed_anim[fret + !m_drums].get());
			// The note head
			{
				ColorTrans c(window, colorize(color(fret), time)); // Get a color for the fret and adjust it if GodMode is on
				m_button.dimensions.center(time2y(0.0f)).middle(x);
				m_button.draw(window);
			}
			// Tap note indicator
			{
				ColorTrans c(window, Color(l, l, l));
				m_tap.dimensions = m_button.dimensions;
				m_tap.draw(window);
			}
		}
	}

	drawNotes(time);

	auto drumsKickButtonId = to_underlying(input::ButtonId::DRUMS_KICK);
	// Draw flames
	for (unsigned fret = 0; fret < m_pads; ++fret) { // Loop through the frets
		if (m_drums && fret == drumsKickButtonId) { // Skip bass drum
			m_flames[fret].clear(); continue;
		}
		Texture* ftex = &m_flame;
		if (m_starpower.get() > 0.01f) ftex = &m_flame_godmode;
		float x = getFretX(fret);
		for (auto it = m_flames[fret].begin(); it != m_flames[fret].end();) {
			float flameAnim = static_cast<float>(it->get());
			if (flameAnim == 1.0f) {
				it = m_flames[fret].erase(it);
				continue;
			}
			float h = flameAnim * 4.0f * fretWid;
			UseTexture tblock(window, *ftex);
			glutil::VertexArray va;
			glmath::vec4 c(1.0f, 1.0f, 1.0f, 1.0f - flameAnim);
			va.texCoord(0.0f, 1.0f).color(c).vertex(x - fretWid, time2y(0.0f), 0.0f);
			va.texCoord(1.0f, 1.0f).color(c).vertex(x + fretWid, time2y(0.0f), 0.0f);
			va.texCoord(0.0f, 0.0f).color(c).vertex(x - fretWid, time2y(0.0f), h);
			va.texCoord(1.0f, 0.0f).color(c).vertex(x + fretWid, time2y(0.0f), h);
			va.draw();
			++it;
		}
	}
	// Accuracy indicator
	UseShader us(getShader(window, "color"));
	float maxsize = 1.5f;
	float thickness = 0.12f;
	float x = -2.5f - thickness;
	float y = time2y(0.0f);
	float alpha = static_cast<float>(m_errorMeterFade.get());
	float bgcol = static_cast<float>(m_errorMeterFlash.get());
	// Indicator background
	{
		glmath::vec4 c(bgcol, bgcol, bgcol, 0.6f * alpha);
		glutil::VertexArray va;
		va.color(c).texCoord(0.0f,0.0f).vertex(x - thickness, y + maxsize);
		va.color(c).texCoord(0.0f,0.0f).vertex(x, y + maxsize);
		va.color(c).texCoord(0.0f,0.0f).vertex(x - thickness, y - maxsize);
		va.color(c).texCoord(0.0f,0.0f).vertex(x, y - maxsize);
		va.draw();
	}
	// Indicator bar
	float error = static_cast<float>(m_errorMeter.get());
	if (error != 0.0f) {
		if (m_errorMeter.get() == m_errorMeter.getTarget()) m_errorMeter.setTarget(0.0);
		float y1 = 0.0f, y2 = 0.0f;
		glmath::vec4 c;
		if (error > 0.0f) { c = glmath::vec4(0.0f, 1.0f, 0.0f, alpha); y2 = -maxsize * error; }
		else { c = glmath::vec4(1.0f, 0.0f, 0.0f, alpha); y1 = -maxsize * error; }
		glutil::VertexArray va;
		va.color(c).texCoord(0.0f,0.0f).vertex(x - thickness, y1 + y);
		va.color(c).texCoord(0.0f,0.0f).vertex(x, y1 + y);
		va.color(c).texCoord(0.0f,0.0f).vertex(x - thickness, y2 + y);
		va.color(c).texCoord(0.0f,0.0f).vertex(x, y2 + y);
		va.draw();
	}
}

/// Main drawing function (projection, neck, cursor...)
void GuitarGraph::draw(double time) {
	glutil::GLErrorChecker ec("GuitarGraph::draw");
	auto& window = m_game.getWindow();
	ViewTrans view(window, static_cast<float>(m_cx.get()), 0.0f, 0.75f);  // Apply a per-player local perspective

	drawNeckStuff(time);

	if (m_neckglowColor.w > 0.0f) {
		// Neck glow drawing
		using namespace glmath;
		ColorTrans c(window, glmath::diagonal(m_neckglowColor));
		m_neckglow.dimensions.screenBottom(0.0f).middle().fixedWidth(neckWidth());
		m_neckglow.draw(window);
	}

	drawInfo(time); // Go draw some texts and other interface stuff
}

/// Draws a single note
/// The times passed are normalized to [past, future]
void GuitarGraph::drawNote(unsigned fret, Color color, double tBeg, double tEnd, float whammy, bool tappable, bool hit, double hitAnim, double releaseTime) {
	auto& window = m_game.getWindow();
	float x = getFretX(fret);
	unsigned drumsKickButtonId = to_underlying(input::ButtonId::DRUMS_KICK);
	if (m_drums && fret == drumsKickButtonId) { // Bass drum? That's easy
		if (hit || hitAnim > 0) return;	// Hide it if it's hit
		color.a = time2a(static_cast<float>(tBeg));
		{
			ColorTrans c(window, color);
			drawBar(tBeg, 0.015f);
		}
		return;
	}
	// If the note is hit, limit it to cursor position
	float yBeg = (hit || hitAnim > 0) ? std::min(time2y(0.0f), time2y(static_cast<float>(tBeg))): time2y(static_cast<float>(tBeg));
	float yEnd = time2y(static_cast<float>(tEnd));
	// Long notes
	if (yBeg - 2 * fretWid >= yEnd) {
		// A hold is released? Let it go...
		if (releaseTime == releaseTime && tEnd - releaseTime > 0.1) yBeg = time2y(static_cast<float>(releaseTime));
		// Short note? Render minimum renderable length
		if (yEnd > yBeg - 3 * fretWid) yEnd = yBeg - 3 * fretWid;
		// Skip the fret head
		float y = yBeg + fretWid;
		y -= fretWid;
		float fretY = y;
		color.a = clamp(time2a(static_cast<float>(tBeg))*2.0f,0.0f,1.0f);
		y -= fretWid;
		// Render the middle
		bool doanim = hit || hitAnim > 0.0f; // Enable glow?
		Texture const& tex(doanim ? m_tail_glow : m_tail); // Select texture
		UseTexture tblock(window, tex);
		glutil::VertexArray va;
		double t = m_audio.getPosition() * 10.0; // Get adjusted time value for animation
		vertexPair(va, x, y, color, doanim ? tc(static_cast<float>(y + t)) : 1.0f); // First vertex pair
		while ((y -= fretWid) > yEnd + fretWid) {
			if (whammy > 0.1f) {
				// FIXME: Should use Boost/C++11 random, and use the same seed for both eyes in stereo3d.
				float r1 = float(rand()) / float(RAND_MAX) - 0.5f;
				float r2 = float(rand()) / float(RAND_MAX) - 0.5f;
				vertexPair(va, x+0.2f*(cos(y*whammy)+r1), y, color, tc(static_cast<float>(y + t)), fretWid, 0.1f*(sin(y*whammy)+r2));
			} else vertexPair(va, x, y, color, doanim ? tc(static_cast<float>(y + t)) : 0.5f);
		}
		// Render the end
		y = yEnd + fretWid;
		vertexPair(va, x, y, color, doanim ? tc(static_cast<float>(y + t)) : 0.20f);
		vertexPair(va, x, yEnd, color, doanim ? tc(static_cast<float>(yEnd + t)) : 0.0f);
		glDisable(GL_DEPTH_TEST);
		va.draw();
		glEnable(GL_DEPTH_TEST);
		// Render the fret object
		{
			ColorTrans c(window, color);
			m_fretObj.draw(window, x, fretY, 0.0f);
		}
	} else {
		// Too short note: only render the ring
		if (hitAnim > 0.0f && tEnd <= maxTolerance) {
			float s = static_cast<float>(1.0f - hitAnim);
			color.a = s;
			{
				ColorTrans c(window, color);
				m_fretObj.draw(window, x, yBeg, 0.0f, s);
			}
		} else {
			color.a = clamp(time2a(static_cast<float>(tBeg))*2.0f,0.0f,1.0f);
			{
				ColorTrans c(window, color);
				m_fretObj.draw(window, x, yBeg, 0.0f);
			}
		}
	}
	// Hammer note caps
	if (tappable) {
		float l = std::max(0.3f, static_cast<float>(m_correctness.get()));
		float s = static_cast<float>(1.0f - hitAnim);
		ColorTrans c(window, Color(l, l, l, s));
		m_tappableObj.draw(window, x, yBeg, 0.0f, s);
	}
}

/// Draws a drum fill
void GuitarGraph::drawDrumfill(double tBeg, double tEnd) {
	auto& window = m_game.getWindow();

	for (unsigned fret = m_drums; fret < m_pads; ++fret) { // Loop through the frets
		float x = -2.0f + static_cast<float>(fret) - 0.5f * m_drums;
		float yBeg = time2y(static_cast<float>(tBeg));
		float yEnd = time2y(static_cast<float>(tEnd <= future ? tEnd : future));
		float tcEnd = tEnd <= future ? 0.0f : 0.25f;
		Color c = color(fret);
		UseTexture tblock(window, m_tail_drumfill);
		glutil::VertexArray va;
		vertexPair(va, x, yBeg, c, 1.0f); // First vertex pair
		if (std::abs(yEnd - yBeg) > 4.0 * fretWid) {
			float y = yBeg - 2.0f * fretWid;
			vertexPair(va, x, y, c, 0.75f);
			while ((y -= 10.0f * fretWid) > yEnd + 2.0f * fretWid)
				vertexPair(va, x, y, c, 0.5f);
			vertexPair(va, x, yEnd + 2.0f * fretWid, c, 0.25f);
		}
		vertexPair(va, x, yEnd, c, tcEnd); // Last vertex pair
		va.draw();
	}
}

/// Draw popups and other info texts
void GuitarGraph::drawInfo(double time) {
	auto& window = m_game.getWindow();
	// Draw score/streak counters
	if (!menuOpen()) {
		using namespace glmath;
		Transform trans(window, translate(vec3(0.0f, 0.0f, -0.5f)));  // Add some depth
		double w = neckWidth();
		float xcor = static_cast<float>(0.53f * w);
		float h = static_cast<float>(0.15f * w);
		// Draw scores
		{
			ColorTrans c(window, Color(0.1f, 0.3f, 1.0f, 0.9f));
			m_scoreText->render(fmt::format("{:04d}", getScore()));
			m_scoreText->dimensions().middle(-xcor).fixedHeight(h).screenBottom(-0.22f);
			m_scoreText->draw(window);
		}
		// Draw streak counter
		{
			ColorTrans c(window, Color(0.6f, 0.6f, 0.7f, 0.95f));
			m_streakText->render(std::to_string(unsigned(m_streak)) + "/"
			  + std::to_string(unsigned(m_longestStreak)));
			m_streakText->dimensions().middle(-xcor).fixedHeight(h*0.75f).screenBottom(-0.18f);
			m_streakText->draw(window);
		}
	}
	// Status text at the bottom
	{
		ColorTrans c(window, Color::alpha(static_cast<float>(std::abs(std::fmod(time, 1.0) - 0.5) * 2.0)));
		if (canActivateStarpower()) {
			m_text.dimensions.screenBottom(-0.02f).middle(-0.12f);
			if (!m_drums)
				m_text.draw(window, _("God Mode Ready!"));
			else if (m_dfIt != m_drumfills.end() && time >= m_dfIt->begin && time <= m_dfIt->end)
				m_text.draw(window, _("Drum Fill!"));
		} else if (m_solo) {
			m_text.dimensions.screenBottom(-0.02f).middle(-0.03f);
			m_text.draw(window, _("Solo!"));
		}
	}
	drawPopups();
}

/// Draw a bar for drum bass pedal/note
void GuitarGraph::drawBar(double time, float h) {
	auto& window = m_game.getWindow();
	UseShader shader(getShader(window, "color"));
	glutil::VertexArray va;

	va.normal(0.0f, 1.0f, 0.0f).texCoord(0,0).vertex(-2.5f, time2y(static_cast<float>(time + h)));
	va.normal(0.0f, 1.0f, 0.0f).texCoord(0,0).vertex(2.5f, time2y(static_cast<float>(time + h)));
	va.normal(0.0f, 1.0f, 0.0f).texCoord(0,0).vertex(-2.5f, time2y(static_cast<float>(time - h)));
	va.normal(0.0f, 1.0f, 0.0f).texCoord(0,0).vertex(2.5f, time2y(static_cast<float>(time - h)));

	va.draw();
}

bool GuitarGraph::updateTom(unsigned int tomTrack, int fretId) {
	auto tomTrackIt = m_track_index->second->nm.find(tomTrack);
	if (tomTrackIt == m_track_index->second->nm.end()) return false;  // Track not found
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
void GuitarGraph::updateChords() {
	m_chords.clear();
	m_solos.clear();
	m_drumfills.clear();
	m_scoreFactor = 0;
	NoteMap const& nm = m_track_index->second->nm;

	Durations::size_type pos[5] = {}, size[5] = {};
	Durations const* durations[5] = {};
	auto currentLevel = to_underlying(m_level);
	for (unsigned fret = 0; fret < m_pads; ++fret) {
		unsigned basepitch = diffv[currentLevel].basepitch;
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
		if (!m_drums || m_song.hasBRE)
			m_scoreFactor += 50.0f * (m_drumfills.back().end - m_drumfills.back().begin);
	}
	m_dfIt = m_drumfills.end();

	// Normalize maximum score factor
	m_scoreFactor = 10000.0f / m_scoreFactor;
}
