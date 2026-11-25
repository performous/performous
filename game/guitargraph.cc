#include "guitargraph.hh"

#include "config.hh"
#include "configuration.hh"
#include "fs.hh"
#include "log.hh"
#include "platform.hh"
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
	const float g_angle = 1.4f; // How many radians to rotate the fretboards
	const float past = GuitarEngine::past;
	const float future = GuitarEngine::future;
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
	const double maxTolerance = GuitarEngine::maxTolerance;
	const float drumfill_min_rate = GuitarEngine::drumfill_min_rate;
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
  m_track_index(m_instrumentTracks.end()),
  m_engine(*this, m_drums, config["game/drum_promode"].b(), song.hasBRE),
  m_errorMeter(0.0, 2.0),
  m_errorMeterFlash(0.0, 4.0),
  m_errorMeterFade(0.0, 0.333),
  m_drumJump(0.0, 12.0)
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
			if (Difficulty(level) == this->level()) cur = static_cast<unsigned short>(ol.size()-1u);
		}
	}
	m_selectedDifficulty = ConfigItem(ol); // Create a ConfigItem from the option list
	m_selectedDifficulty.select(cur); // Set the selection to current level
	m_menu.add(MenuOption("", _("Select difficulty"))).changer(m_selectedDifficulty); // MenuOption that cycles the options
	m_menu.back().setDynamicName(m_difficultyOpt); // Set the title to be dynamic
}

namespace {
	GuitarEngine::Settings readSettings() {
		GuitarEngine::Settings settings;
		settings.missPenalty = config["game/instrument_miss_penalty"].b();
		settings.missVolumePercent[to_underlying(GuitarEngine::Difficulty::KIDS)] = config["audio/miss_volume_kids"].ui();
		settings.missVolumePercent[to_underlying(GuitarEngine::Difficulty::SUPAEASY)] = config["audio/miss_volume_easy"].ui();
		settings.missVolumePercent[to_underlying(GuitarEngine::Difficulty::EASY)] = config["audio/miss_volume_medium"].ui();
		settings.missVolumePercent[to_underlying(GuitarEngine::Difficulty::MEDIUM)] = config["audio/miss_volume_hard"].ui();
		settings.missVolumePercent[to_underlying(GuitarEngine::Difficulty::AMAZING)] = config["audio/miss_volume_expert"].ui();
		return settings;
	}
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
	if (dir >= 0) {
		if (++m_track_index == m_instrumentTracks.end()) m_track_index = m_instrumentTracks.begin();
	} else {
		if (m_track_index == m_instrumentTracks.begin()) m_track_index = m_instrumentTracks.end();
		--m_track_index;
	}
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
	return _(GuitarEngine::difficultyName(level()));
}

/// Get a string id for track and difficulty
std::string GuitarGraph::getModeId() const {
	return m_track_index->first + " - " + GuitarEngine::difficultyName(level());
}

/// Cycle through difficulties
void GuitarGraph::changeDifficulty(int dir) {
	auto maxLevel = to_underlying(Difficulty::COUNT);
	auto currentLevel = to_underlying(this->level());
	for (int level = ((int)this->level() + dir) % maxLevel; level != currentLevel;
	  level = (level+dir) % maxLevel)
		if (difficulty(Difficulty(level))) return;
}

/// Find an initial difficulty level to use
void GuitarGraph::difficultyAuto(bool tryKeep) {
	if (tryKeep && difficulty(Difficulty(level()))) return;
	auto maxLevel = to_underlying(Difficulty::COUNT);
	for (int level = 0; level < maxLevel; ++level) if (difficulty(Difficulty(level))) return;
	throw std::runtime_error("No difficulty levels found for track " + m_track_index->first);
}

/// Attempt to use a given difficulty level
bool GuitarGraph::difficulty(Difficulty level, bool check_only) {
	InstrumentTrack const& track = *m_track_index->second;
	// Check if the difficulty level is available
	if (!GuitarEngine::hasDifficulty(track, level)) return false;
	if (check_only) return true;
	if (!m_engine.setDifficulty(track, level)) return false;
	m_recorder.chart(m_track_index->first, static_cast<unsigned>(to_underlying(level)));
	return true;
}

/// Core engine
void GuitarGraph::engine() {
	double time = m_audio.getPosition();
	time -= config["audio/controller_delay"].f();
	doUpdates();
	m_recorder.frame(time);
	m_engine.settings = readSettings();
	m_engine.beginFrame(time);
	// Countdown to start
	double const notesBegin = m_engine.getNotesBeginTime();
	handleCountdown(time, time < notesBegin ? notesBegin : m_jointime+1);
	// Handle all events
	for (input::Event ev; m_dev->getEvent(ev); ) {
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
				if (!m_drums && num > 0) { // Only guitar frets can be held
					m_engine.setPressed(num, static_cast<bool>(ev.value));
					m_recorder.pressed(num, m_engine.pressed(num));
				}
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
			else if (std::stoi(m_selectedDifficulty.so()) != to_underlying(level()))
				difficulty(Difficulty(std::stoi(m_selectedDifficulty.so())));
			else if (m_rejoin.b()) { unjoin(); m_engine.resetScore(); m_recorder.rejoin(); setupJoinMenu(); m_dev->pushEvent(input::Event()); /* FIXME: HACK! */ }
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

		m_engine.input(time, ev);
		m_recorder.input(to_underlying(ev.button.id), ev.value, m_engine.whammy());
	}
	// Handle fret hold markers
	if (!m_drums) for (unsigned num = 0; num < max_panels; ++num) m_pressed_anim[num].setTarget(m_engine.pressed(num) ? 1.0 : 0.0);
	m_engine.update(time);
	m_recorder.state(m_engine.score(), m_engine.streak(), m_engine.longestStreak(), m_engine.starmeter());
}

void GuitarGraph::saveReport() {
	auto deviceType = [this]() -> std::string {
		switch (m_dev->source.type) {
			case input::SourceType::KEYBOARD: return "keyboard";
			case input::SourceType::JOYSTICK: return "joystick";
			case input::SourceType::MIDI: return "midi";
			default: return "other";
		}
	};
	auto os = []() -> std::string {
		switch (Platform::currentOS()) {
			case Platform::HostOS::OS_WIN: return "windows";
			case Platform::HostOS::OS_LINUX: return "linux";
			case Platform::HostOS::OS_MAC: return "macos";
			case Platform::HostOS::OS_BSD: return "bsd";
			default: return "unix";
		}
	};
	try {
		nlohmann::json charts = nlohmann::json::array();
		for (auto const& c: m_recorder.charts()) {
			auto it = m_instrumentTracks.find(c.track);
			if (it == m_instrumentTracks.end()) continue;
			charts.push_back({{"track", c.track}, {"difficulty", GuitarEngine::difficultyName(Difficulty(c.level))}, {"level", c.level}, {"notes", chartToJson(*it->second, GuitarEngine::basePitch(Difficulty(c.level)))}});
		}
		fs::path const& notesFile = m_song.midifilename.empty() ? m_song.filename : m_song.midifilename;
		nlohmann::json report = {
			{"format", "performous-game-report"},
			{"formatVersion", 1},
			{"performous", VERSION},
			{"environment", {{"os", os()}}},
			{"song", {{"fingerprint", songFingerprint(notesFile)}, {"format", notesFile.extension().string()}, {"hasBRE", m_song.hasBRE}}},
			{"players", {{
				{"instrument", m_drums ? "drums" : "guitar"},
				{"device", deviceType()},
				{"lefty", m_leftymode.b()},
				{"proMode", m_engine.proMode()},
				{"settings", {
					{"audio/controller_delay", config["audio/controller_delay"].f()},
					{"game/instrument_miss_penalty", config["game/instrument_miss_penalty"].b()},
					{"audio/miss_volume_kids", config["audio/miss_volume_kids"].ui()},
					{"audio/miss_volume_easy", config["audio/miss_volume_easy"].ui()},
					{"audio/miss_volume_medium", config["audio/miss_volume_medium"].ui()},
					{"audio/miss_volume_hard", config["audio/miss_volume_hard"].ui()},
					{"audio/miss_volume_expert", config["audio/miss_volume_expert"].ui()},
				}},
				{"charts", charts},
				{"events", m_recorder.entries()},
			}}},
		};
		fs::path file = saveGameReport(report);
		m_game.flashMessage(_("Saved for a bug report:") + std::string("\n") + file.filename().string());
	} catch (std::exception const& e) {
		SpdLogger::error(LogSystem::INSTRUMENTS, "Cannot save the game report: {}", e.what());
		m_game.flashMessage(_("Could not save the game for a bug report"));
	}
}

void GuitarGraph::onStarpowerActivated() {
	m_popups.push_back(Popup(_("God Mode\nActivated!"),
	  Color(0.3f, 0.0f, 1.0f), 0.666f, m_popupText.get(), _("Mistakes ignored!"), &m_text));
}

void GuitarGraph::onSoloEnded(unsigned percent) {
	m_popups.push_back(Popup(std::to_string(percent) + " %",
	  Color(0.0f, 0.8f, 0.0f), 1.0f, m_popupText.get()));
}

void GuitarGraph::onBigStreak(int streak) {
	m_popups.push_back(Popup(std::to_string(unsigned(streak)) + "\n" + _("Streak!"),
	  Color(1.0f, 0.0f, 0.0f), 1.0f, m_popupText.get()));
}

void GuitarGraph::onChordPassed(double time) {
	if (!joining(time)) ++m_dead;  // Increment dead counter (but not while joining)
}

void GuitarGraph::onMiss(int fret) {
	if (fret < 0) fret = static_cast<int>(std::rand());
	m_audio.playSample(m_samples[unsigned(fret) % m_samples.size()]);
}

void GuitarGraph::onFlame(unsigned fret) {
	m_flames[fret].push_back(AnimValue(0.0, flameSpd));
	m_flames[fret].back().setTarget(1.0);
}

void GuitarGraph::onDrumKick() {
	m_drumJump.setTarget(1.0);
}

/// New hit for the error indicator
void GuitarGraph::onErrorMeter(double error) {
	error /=  maxTolerance;
	m_errorMeter.setTarget(error, true);
	m_errorMeterFade.setValue(1.0f);
	if (std::abs(error) < 0.10f) m_errorMeterFlash.setValue(1.0f);
}

/// Modify color based on things like GodMode and solos
Color const GuitarGraph::colorize(Color c, double time) const {
	const static Color godmodeC(0.5f, 0.5f, 1.0f); // Color for full GodMode
	const static Color soloC(0.2f, 0.9f, 0.2f); // Color for solo notes
	// Solo? (cannot use m_solo, as time can be in future)
	for (auto const& solo: m_engine.solos()) {
		if (time >= solo.begin && time <= solo.end) { c = soloC; break; }
	}
	float f = static_cast<float>(m_engine.starpower().get());
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
	m_engine.updateLookahead(time);  // FIXME: gameplay rules that depend on drawing
	glutil::UseDepthTest depthtest;
	auto const& drumfills = m_engine.drumfills();
	auto const dfIt = m_engine.drumfillIt();
	// Draw drum fills / Big Rock Endings
	bool drumfill = dfIt != drumfills.end() && dfIt->begin - time <= future;
	if (drumfill) {
		drawDrumfill(dfIt->begin - time, dfIt->end - time);
		// If it is a drum fill (not BRE), draw the final note
		if (m_drums && (!m_song.hasBRE || (dfIt != (--drumfills.end())))) {
			Color c(0.7f, 0.7f, 0.7f);
			if (m_engine.drumfillHits() >= drumfill_min_rate * (dfIt->end - dfIt->begin))
				c = colorize(color(4), dfIt->end);
			drawNote(4, c, dfIt->end - time, dfIt->end - time, 0.0f, false, false, 0.0, 0.0);
		}
	}
	if (time != time) return;  // Check that time is not NaN

	glmath::vec4 neckglow{};  // Used for calculating the average neck color

	// Iterate chords
	for (auto const& chord: m_engine.chords()) {
		float tBeg = static_cast<float>(chord.begin - time);
		float tEnd = static_cast<float>(m_drums ? tBeg : chord.end - time);
		if (tBeg > future) break;
		if (tEnd < past) continue;
		// Don't show past chords when rewinding
		if (chord.passed || (tBeg > maxTolerance && chord.status > 0)) continue;
		// Skip notes during drum fills / BREs
		if (drumfill && chord.begin >= dfIt->begin - maxTolerance
		  && chord.begin <= dfIt->end + maxTolerance) continue;
		// Loop through the frets
		for (unsigned fret = 0; fret < m_pads; ++fret) {
			if (!chord.fret[fret] || (tBeg > maxTolerance && chord.releaseTimes[fret] > 0)) continue;
			if (tEnd > future) tEnd = future;
			unsigned event = m_engine.noteEvent(chord.dur[fret]);
			float glow = 0.0f;
			float whammy = 0.0f;
			if (event > 0) {
				glow = static_cast<float>(m_engine.events()[event - 1].glow.get());
				whammy = static_cast<float>(m_engine.events()[event - 1].whammy.get());
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
		if (m_engine.starpower().get() > 0.01f) ftex = &m_flame_godmode;
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
	// Drawing changes game state (passed chords, notes under drum fills) and reads animated values
	m_recorder.draw(time);
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
		float l = std::max(0.3f, static_cast<float>(m_engine.correctness().get()));
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
			m_streakText->render(std::to_string(unsigned(m_engine.streak())) + "/"
			  + std::to_string(unsigned(m_engine.longestStreak())));
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
			else if (m_engine.drumfillIt() != m_engine.drumfills().end() && time >= m_engine.drumfillIt()->begin && time <= m_engine.drumfillIt()->end)
				m_text.draw(window, _("Drum Fill!"));
		} else if (m_engine.solo()) {
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
