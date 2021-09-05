#include "dancegraph.hh"
#include "song.hh"
#include "i18n.hh"

#include <stdexcept>
#include <algorithm>

namespace {
	// Position mappings for panels
	static const int mapping4[max_panels] = {0, 1, 2, 3,-1,-1,-1,-1,-1,-1};
	static const int mapping5[max_panels] = {0, 1, 3, 4, 2,-1,-1,-1,-1,-1};
	static const int mapping6[max_panels] = {0, 2, 3, 5, 1, 4,-1,-1,-1,-1};
	static const int mapping7[max_panels] = {0, 2, 4, 6, 1, 5, 3,-1,-1,-1};
	static const int mapping8[max_panels] = {0, 3, 4, 7, 1, 6, 2, 5,-1,-1};
	static const int mapping10[max_panels]= {0, 3, 4, 7, 1, 6, 2, 5,-1,-1};
	#if 0 // Here is some dummy gettext calls to populate the dictionary
	_("Beginner") _("Easy") _("Medium") _("Hard") _("Challenge")
	#endif
	const std::string diffv[] = { "Beginner", "Easy", "Medium", "Hard", "Challenge" };
	const float past = -0.3f; // Relative time from cursor that is considered past (out of screen)
	const float future = 2.0f; // Relative time from cursor that is considered future (out of screen)
	const float timescale = 12.0f; // Multiplier to get graphics units from time
	const float texCoordStep = -0.25f; // Four beat lines per beat texture
	// Note: t is difference from playback time so it must be in range [past, future]
	float time2y(float t) { return timescale * (t - past) / (future - past); }
	float time2a(float t) {
		float a = clamp(1.0 - t / future); // Note: we want 1.0 alpha already at zero t.
		return std::pow(a, 0.8f); // Nicer curve
	}
	const double maxTolerance = 0.15; // Maximum error in seconds
	int getNextBigStreak(int prev) { return prev + 10; }

	/// Get an accuracy value [0, 1] for the error offset (in seconds)
	double accuracy(double error) { return 1.0 - (std::abs(error) / maxTolerance); }

	/// Gives points based on error from a perfect hit
	double points(double error) {
		double ac = accuracy(error);
		if (ac > .90) return 50.0;  // Perfect
		if (ac > .80) return 40.0;  // Excellent
		if (ac > .70) return 30.0;  // Great
		if (ac > .60) return 20.0;  // Good
		if (ac > .40) return 15.0;  // OK
		if (ac > .20) return 10.0;  // Late/Early
		return 5.0;  // Way off
	}

	std::string getRank(double error) {
		double ac = accuracy(error);
		if (error < 0.0) {
			if (ac > .90) return _("Perfect!");
			if (ac > .80) return _("Excellent!-");
			if (ac > .70) return _("Great!-");
			if (ac > .60) return _(" Good!- ");
			if (ac > .40) return _("  OK!-  ");
			if (ac > .20) return _("Late!-");
			return _("Way off!");
		} else {
			if (ac > .90) return _("Perfect!");
			if (ac > .80) return _("-Excellent!");
			if (ac > .70) return _("-Great!");
			if (ac > .60) return _(" -Good! ");
			if (ac > .40) return _("  -OK!  ");
			if (ac > .20) return _("-Early!");
			return _("Way off!");
		}
	}

	struct lessEnd {
		bool operator()(const DanceNote& left, const DanceNote& right) {
			return left.note.end < right.note.end;
		}
	};
}


/// Constructor
DanceGraph::DanceGraph(Audio& audio, Song const& song, input::DevicePtr dev):
  InstrumentGraph(audio, song, dev),
  m_level(DanceDifficulty::BEGINNER),
  m_beat(findFile("dancebeat.svg")),
  m_arrows(findFile("arrows.svg")),
  m_arrows_cursor(findFile("arrows_cursor.svg")),
  m_arrows_hold(findFile("arrows_hold.svg")),
  m_mine(findFile("mine.svg")),
  m_insideStop()
{
	// Initialize some arrays
	for (size_t i = 0; i < max_panels; i++) {
		m_activeNotes[i] = m_notes.end();
		m_pressed_anim[i] = AnimValue(0.0, 4.0);
		m_arrow_map[i] = -1;
	}

	if(m_song.danceTracks.empty())
		throw std::runtime_error("Could not find any dance tracks.");
	changeTrack(0); // Get an initial game mode and notes for it
	setupJoinMenu(); // Finally setup the menu
}


void DanceGraph::setupJoinMenu() {
	m_menu.clear();
	updateJoinMenu();
	// Populate root menu
	m_menu.add(MenuOption(_("Ready!"), _("Start performing!")));
	{ // Create track selector
		ConfigItem::OptionList ol;
		int i = 0, cur = 0;
		// Add tracks to option list
		for (auto it = m_song.danceTracks.begin(); it != m_song.danceTracks.end(); ++it, ++i) {
			ol.push_back(it->first);
			if (m_gamingMode == it->first) cur = i; // Find the index of current track
		}
		m_selectedTrack = ConfigItem(ol); // Create a ConfigItem from the option list
		m_selectedTrack.select(cur); // Set the selection to current level
		m_menu.add(MenuOption("", _("Select track")).changer(m_selectedTrack)); // MenuOption that cycles the options
		m_menu.back().setDynamicName(m_trackOpt); // Set the title to be dynamic
	}
	{ // Create difficulty opt
		ConfigItem::OptionList ol;
		int i = 0, cur = 0;
		// Add difficulties to the option list
		for (int level = 0; level < to_underlying(DanceDifficulty::COUNT); ++level) {
			if (difficulty(DanceDifficulty(level), true)) {
				ol.push_back(std::to_string(level));
				if (DanceDifficulty(level) == m_level) cur = i;
				++i;
			}
		}
		m_selectedDifficulty = ConfigItem(ol); // Create a ConfigItem from the option list
		m_selectedDifficulty.select(cur); // Set the selection to current level
		m_menu.add(MenuOption("", _("Select difficulty")).changer(m_selectedDifficulty)); // MenuOption that cycles the options
		m_menu.back().setDynamicName(m_difficultyOpt); // Set the title to be dynamic
	}
	m_menu.add(MenuOption(_("Quit"), _("Exit to song browser")).screen("Songs"));
}

void DanceGraph::updateJoinMenu() {
	m_trackOpt = getTrack();
	m_difficultyOpt = getDifficultyString();
}

/// Attempt to select next/previous game mode
void DanceGraph::changeTrack(int direction) {
	// Cycling
	if (direction == 0) {
		m_curTrackIt = m_song.danceTracks.find("dance-single");
		if (m_curTrackIt == m_song.danceTracks.end())
			m_curTrackIt = m_song.danceTracks.begin();
	} else if (direction > 0) {
		++m_curTrackIt;
		if (m_curTrackIt == m_song.danceTracks.end()) m_curTrackIt = m_song.danceTracks.begin();
	} else if (direction < 0) {
		if (m_curTrackIt == m_song.danceTracks.begin()) m_curTrackIt = (--m_song.danceTracks.end());
		else --m_curTrackIt;
	}
	finalizeTrackChange();
}

/// Attempt to select a specific game mode
void DanceGraph::setTrack(const std::string& track) {
	auto it = m_song.danceTracks.find(track);
	if (it == m_song.danceTracks.end()) return;
	m_curTrackIt = it;
	finalizeTrackChange();
}

void DanceGraph::finalizeTrackChange() {
	// Determine how many arrow lines are needed
	m_gamingMode = m_curTrackIt->first;
	std::string gm = m_gamingMode;
	if (gm == "dance-single") { m_pads = 4; std::copy(mapping4, mapping4+max_panels, m_arrow_map); }
	else if (gm == "dance-double") { m_pads = 8; std::copy(mapping8, mapping8+max_panels, m_arrow_map); }
	else if (gm == "dance-couple") { m_pads = 8; std::copy(mapping8, mapping8+max_panels, m_arrow_map); }
	else if (gm == "dance-solo") { m_pads = 6; std::copy(mapping6, mapping6+max_panels, m_arrow_map); }
	else if (gm == "pump-single") { m_pads = 5 ; std::copy(mapping5, mapping5+max_panels, m_arrow_map); }
	else if (gm == "pump-double") { m_pads = 10; std::copy(mapping10, mapping10+max_panels, m_arrow_map); }
	else if (gm == "pump-couple") { m_pads = 10; std::copy(mapping10, mapping10+max_panels, m_arrow_map); }
	else if (gm == "ez2-single") { m_pads = 5; std::copy(mapping5, mapping5+max_panels, m_arrow_map); }
	else if (gm == "ez2-double") { m_pads = 10; std::copy(mapping10, mapping10+max_panels, m_arrow_map); }
	else if (gm == "ez2-real") { m_pads = 7; std::copy(mapping7, mapping7+max_panels, m_arrow_map); }
	else if (gm == "para-single") { m_pads = 5; std::copy(mapping5, mapping5+max_panels, m_arrow_map); }
	else throw std::runtime_error("Unknown track " + gm);

	changeDifficulty(0); // Construct new notes
	setupJoinMenu();
	m_menu.select(1); // Restore selection to the track item
}

/// Get the track string
std::string DanceGraph::getTrack() const {
	return _(m_gamingMode.c_str());
}

/// Get the difficulty as displayable string
std::string DanceGraph::getDifficultyString() const {
	return _(diffv[to_underlying(m_level)].c_str());
}

/// Get a string id for track and difficulty
std::string DanceGraph::getModeId() const {
	return m_gamingMode + " - " + diffv[to_underlying(m_level)] + (isKeyboard() ? " (kbd)" : "");
}

/// Attempt to change the difficulty by a step
void DanceGraph::changeDifficulty(int delta) {
	int newLevel = to_underlying(m_level) + delta;
	if(newLevel >= to_underlying(DanceDifficulty::COUNT) || newLevel < 0) return; // Out of bounds
	auto it = m_song.danceTracks.find(m_gamingMode);
	if(it->second.find((DanceDifficulty)newLevel) != it->second.end())
		difficulty((DanceDifficulty)newLevel);
	else
		changeDifficulty(delta + (delta < 0 ? -1 : 1));
}

/// Select a difficulty and construct DanceNotes and score normalizer for it
bool DanceGraph::difficulty(DanceDifficulty level, bool check_only) {
	// TODO: error handling
	DanceDifficultyMap const& ddm = m_song.danceTracks.find(m_gamingMode)->second;
	if (ddm.find(level) == ddm.end()) return false;	else if (check_only) return true;
	m_notes.clear();
	DanceTrack const& track = ddm.find(level)->second;
	for (auto const& n: track.notes) m_notes.push_back(DanceNote(n));
	std::sort(m_notes.begin(), m_notes.end(), lessEnd()); // for engine's iterators
	m_notesIt = m_notes.begin();
	m_level = level;
	for (auto& noteIt: m_activeNotes) noteIt = m_notes.end();
	m_scoreFactor = 1;
	if (!m_notes.empty()) m_scoreFactor = 10000.0 / (50 * m_notes.size()); // maxpoints / (notepoint * notes)
	updateJoinMenu();
	return true;
}

/// Handles input and some logic
void DanceGraph::engine() {
	double time = m_audio.getPosition();
	time -= config["audio/controller_delay"].f();
	doUpdates();
	// Handle stops
	bool outsideStop = true;
	for (auto const& stop: m_song.stops) {
		if (stop.first >= time) break;
		if (time < stop.first + stop.second) {  // Inside stop
			time = stop.first;
			if (!m_insideStop) {
				m_popups.push_back(Popup(_("STOP!"),  Color(1.0, 0.8, 0.0), 2.0, m_popupText.get()));
				m_insideStop = true;
			}
			outsideStop = false;
			break;
		}
		time -= stop.second;
	}
	if (outsideStop && m_insideStop) m_insideStop = false;
	bool difficulty_changed = false;
	// Handle all events
	for (input::Event ev; m_dev->getEvent(ev); ) {
		m_dead = 0; // Keep alive
		// Menu keys
		if (menuOpen() && ev.value != 0.0) {
			if (ev.nav == input::NavButton::NAV_START || ev.nav == input::NavButton::NAV_CANCEL) m_menu.close();
			else if (ev.nav == input::NavButton::NAV_RIGHT) m_menu.action(1);
			else if (ev.nav == input::NavButton::NAV_LEFT) m_menu.action(-1);
			else if (ev.nav == input::NavButton::NAV_UP) m_menu.move(-1);
			else if (ev.nav == input::NavButton::NAV_DOWN) m_menu.move(1);
			difficulty_changed = true;
			// See if anything changed
			if (m_selectedTrack.so() != m_gamingMode) setTrack(m_selectedTrack.so());
			else if (std::stoi(m_selectedDifficulty.so()) != to_underlying(m_level))
				difficulty(DanceDifficulty(std::stoi(m_selectedDifficulty.so())));
			else if (m_rejoin.b()) { unjoin(); setupJoinMenu(); m_dev->pushEvent(input::Event()); /* FIXME: HACK? */ }
			// Sync dynamic stuff
			updateJoinMenu();
		// Open Menu
		} else if (!menuOpen() && ev.value != 0.0) {
			if (ev.nav == input::NavButton::NAV_CANCEL || ev.nav == input::NavButton::NAV_START) m_menu.open();
		}
		auto buttonId = to_underlying(ev.button.id);
		if (buttonId < max_panels) {
			// Gaming controls
			if (ev.value == 0.0) {
				m_pressed[buttonId] = false;
				dance(time, ev);
				m_pressed_anim[buttonId].setTarget(0.0);
			} else if (ev.value != 0.0) {
				m_pressed[buttonId] = true;
				dance(time, ev);
				m_pressed_anim[buttonId].setValue(1.0);
			}
		}
	}

	// Countdown to start
	handleCountdown(time, time < getNotesBeginTime() ? getNotesBeginTime() : m_jointime+1);

	// Notes gone by
	for (DanceNotes::iterator& it = m_notesIt; it != m_notes.end() && time > it->note.end + maxTolerance; it++) {
		if(!it->isHit) { // Missed
			if (it->note.type != Note::Type::MINE) m_streak = 0;
		} else { // Hit, add score
			if(it->note.type != Note::Type::MINE) m_score += it->score;
			if(!it->releaseTime) it->releaseTime = time;
		}
		if (!joining(time)) ++m_dead;  // Increment dead counter (but not while joining)
	}
	if (difficulty_changed) m_dead = 0; // if difficulty is changed, m_dead would get incorrect

	// Holding button when mine comes?
	for (auto it = m_notesIt; it != m_notes.end() && time <= it->note.begin + maxTolerance; ++it) {
		if(!it->isHit && it->note.type == Note::Type::MINE && m_pressed[it->note.note] &&
		  it->note.begin >= time - maxTolerance && it->note.end <= time + maxTolerance) {
			it->isHit = true;
			m_score -= points(0);
		}
	}

	// Check if a long streak goal has been reached
	if (m_streak >= getNextBigStreak(m_bigStreak)) {
		m_bigStreak = getNextBigStreak(m_bigStreak);
		m_popups.push_back(Popup(std::to_string(unsigned(m_bigStreak)) + "\n" + _("Streak!"),
		  Color(1.0, 0.0, 0.0), 1.0, m_popupText.get()));
	}
}

/// Handles scoring and such
void DanceGraph::dance(double time, input::Event const& ev) {
	// Handle release events
	if (ev.value == 0.0) {
		auto it = m_activeNotes[to_underlying(ev.button.id)];
		if(it != m_notes.end()) {
			if(!it->releaseTime && it->note.end > time + maxTolerance) {
				it->releaseTime = time;
				it->score = 0;
				m_streak = 0;
			}
		}
		return;
	}

	auto buttonId = to_underlying(ev.button.id);
	// So it was a PRESS event
	for (auto it = m_notesIt; it != m_notes.end() && time <= it->note.end + maxTolerance; ++it) {
		if(!it->isHit && std::abs(time - it->note.begin) <= maxTolerance && buttonId == unsigned(it->note.note)) {
			it->isHit = true;
			if (it->note.type != Note::Type::MINE) {
				it->score = points(it->note.begin - time);
				it->error = it->note.begin - time;
				m_streak++;
				if (m_streak > m_longestStreak) m_longestStreak = m_streak;
			} else { // Mine!
				m_score -= points(0);
				m_streak = 0;
			}
			m_activeNotes[buttonId] = it;
			break;
		}
	}
}


namespace {
	const float arrowSize = 0.4f; // Half width of an arrow
	const float one_arrow_tex_w = 1.0 / 8.0; // Width of a single arrow in texture coordinates

	/// Create a symmetric vertex pair for arrow drawing
	void vertexPair(glutil::VertexArray& va, int arrow_i, float y, float ty) {
		if (arrow_i < 0) {
			// Single thing in a texture (e.g. mine)
			va.texCoord(0.0f, ty).vertex(-arrowSize, y);
			va.texCoord(1.0f, ty).vertex(arrowSize, y);
		} else {
			// Arrow from a texture atlas
			va.texCoord(arrow_i * one_arrow_tex_w, ty).vertex(-arrowSize, y);
			va.texCoord((arrow_i+1) * one_arrow_tex_w, ty).vertex(arrowSize, y);
		}
	}
}

/// Draw a dance pad icon using the given texture
void DanceGraph::drawArrow(int arrow_i, Texture& tex, float ty1, float ty2) {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(tex.type(), tex.id());
	glutil::VertexArray va;
	vertexPair(va, arrow_i, -arrowSize, ty1);
	vertexPair(va, arrow_i,  arrowSize, ty2);
	va.draw();
}

/// Draws the dance graph
void DanceGraph::draw(double time) {
	for (auto const& stop: m_song.stops) {
		if (stop.first >= time) break;
		if (time < stop.first + stop.second) { time = stop.first; break; } // Inside stop
		time -= stop.second;
	}

	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	dimensions.screenTop().middle(m_cx.get()).stretch(m_width.get(), 1.0);
	ViewTrans view(0.5 * (dimensions.x1() + dimensions.x2()), 0.0, 0.75);  // Apply a per-player local perspective
	{
		using namespace glmath;
		// Some matrix magic to get the viewport right
		float temp_s = dimensions.w() / 8.0f; // Allow for 8 pads to fit on a track
		Transform trans(translate(vec3(0.0f, dimensions.y1(), 0.0)) * scale(temp_s));

		// Draw the "neck" graph (beat lines)
		drawBeats(time);

		// Arrows on cursor
		{
			UseShader us(getShader("dancenote"));
			m_uniforms.clock = static_cast<float>(time);
			m_uniforms.noteType = 0;
			m_uniforms.scale = getScale();
			for (unsigned arrow_i = 0; arrow_i < m_pads; ++arrow_i) {
				float l = m_pressed_anim[arrow_i].get();
				m_uniforms.hitAnim = l;
				m_uniforms.position = glmath::vec2(panel2x(arrow_i), time2y(0.0));
				glBufferSubData(GL_UNIFORM_BUFFER, m_uniforms.offset(), m_uniforms.size(), &m_uniforms);
				drawArrow(arrow_i, m_arrows_cursor);
			}
		}

		// Draw the notes
		if (time == time) { // Check that time is not NaN
			for (auto& n: m_notes) {
				if (n.note.end - time < past) continue;
				if (n.note.begin - time > future) continue;
				drawNote(n, time); // Let's just do all the calculating in the sub, instead of passing them as a long list
			}
		}
	}
	drawInfo(time, dimensions); // Go draw some texts and other interface stuff
}

void DanceGraph::drawBeats(double time) {
	UseTexture tex(m_beat);
	glutil::VertexArray va;
	float texCoord = 0.0f;
	float tBeg = 0.0f, tEnd;
	float w = 0.5 * m_pads * getScale();
	for (auto it = m_song.beats.begin(); it != m_song.beats.end() && tBeg < future; ++it, texCoord += texCoordStep, tBeg = tEnd) {
		tEnd = *it - time;
		//if (tEnd < past) continue;
		/*if (tEnd > future) {
			// Crop the end off
			texCoord -= texCoordStep * (tEnd - future) / (tEnd - tBeg);
			tEnd = future;
		}*/
		glmath::vec4 c(1.0f, 1.0f, 1.0f, time2a(tEnd));
		va.color(c).normal(0.0f, 1.0f, 0.0f).texCoord(0.0f, texCoord).vertex(-w, time2y(tEnd));
		va.color(c).normal(0.0f, 1.0f, 0.0f).texCoord(1.0f, texCoord).vertex(w, time2y(tEnd));
	}
	va.draw();
}

/// Draws a single note (or hold)
void DanceGraph::drawNote(DanceNote& note, double time) {
	float tBeg = note.note.begin - time;
	float tEnd = note.note.end - time;
	int arrow_i = note.note.note;
	bool mine = note.note.type == Note::Type::MINE;
	float x = panel2x(arrow_i);
	float yBeg = time2y(tBeg);
	float yEnd = time2y(tEnd);

	// Did we hit it?
	if (note.isHit && (note.releaseTime > 0.0 || std::abs(tEnd) < maxTolerance) && note.hitAnim.getTarget() == 0.0) {
		if (mine) note.hitAnim.setRate(1.0);
		note.hitAnim.setTarget(1.0, false);
	}
	double glow = note.hitAnim.get();

	{
		UseShader us(getShader("dancenote"));
		m_uniforms.hitAnim = static_cast<float>(glow);
		m_uniforms.clock = static_cast<float>(time);
		m_uniforms.scale = getScale();

		if (yEnd - yBeg > arrowSize) {
			// Draw holds
			if (note.isHit && note.releaseTime <= 0) { // The note is being held down
				yBeg = std::max(time2y(0.0), yBeg);
				yEnd = std::max(time2y(0.0), yEnd);
			}
			if (note.releaseTime > 0) yBeg = time2y(note.releaseTime - time); // Oh noes, it got released!
			m_uniforms.noteType = 2;
			m_uniforms.position = glmath::vec2(x, yBeg);
			glBufferSubData(GL_UNIFORM_BUFFER, m_uniforms.offset(), m_uniforms.size(), &m_uniforms);
			// Draw begin
			drawArrow(arrow_i, m_arrows_hold, 0.0f, 1.0f/3.0f);
			if (yEnd - yBeg > 0) {
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(m_arrows_hold.type(), m_arrows_hold.id());
				glutil::VertexArray va;
				// Middle
				vertexPair(va, arrow_i, arrowSize, 1.0f/3.0f);
				float l = (yEnd - yBeg) / getScale();
				float yMid = std::max(l-arrowSize, arrowSize);
				vertexPair(va, arrow_i, yMid, 2.0f/3.0f);
				// End
				vertexPair(va, arrow_i, l, 1.0f);
				va.draw();
			}
		} else {
			// Draw short note
			if (mine && note.isHit) yBeg = time2y(0.0);
			m_uniforms.noteType = (mine ? 3 : 1);
			m_uniforms.position = glmath::vec2(x, yBeg);
			glBufferSubData(GL_UNIFORM_BUFFER, m_uniforms.offset(), m_uniforms.size(), &m_uniforms);
			drawArrow((mine ? -1 : arrow_i), (mine ? m_mine : m_arrows));
		}
	}

	// Draw a text telling how well we hit
	double alpha = 1.0 - glow;
	if (!mine && note.isHit) {
		std::string text;
		if (note.releaseTime <= 0.0 && tBeg < tEnd) { // Is being held down and is a hold note
			text = "HOLD";
			alpha = glow = 1.0;
		} else if (glow > 0.0) { // Released already, display rank
			text = note.score ? getRank(note.error) : "FAIL!";
		}
		if (!text.empty()) {
			double sc = getScale() * 0.6 * arrowSize * (3.0 + glow);
			Transform trans(glmath::translate(glmath::vec3(0.0, 0.0, 0.5 * glow))); // Slightly elevated
			ColorTrans c(Color::alpha(std::sqrt(alpha)));
			m_popupText->render(_(text));
			m_popupText->dimensions().middle(x).center(time2y(0.0)).stretch(sc, sc/2.0);
			m_popupText->draw();
		}
	}
}

/// Draw popups and other info texts
void DanceGraph::drawInfo(double /*time*/, Dimensions dimensions) {
	if (!menuOpen()) {
		// Draw scores
		m_text.dimensions.screenBottom(-0.35).middle(0.32 * dimensions.w());
		m_text.draw(std::to_string(unsigned(getScore())));
		m_text.dimensions.screenBottom(-0.32).middle(0.32 * dimensions.w());
		m_text.draw(std::to_string(unsigned(m_streak)) + "/"
		  + std::to_string(unsigned(m_longestStreak)));
	}
	drawPopups();
}
