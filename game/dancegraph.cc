#include "dancegraph.hh"
#include "instrumentgraph.hh"
#include "fs.hh"
#include "notes.hh"
#include "surface.hh"
#include <boost/lexical_cast.hpp>
#include <stdexcept>
#include <algorithm>

namespace {
	const std::string diffv[] = { "Beginner", "Easy", "Medium", "Hard", "Challenge" };
	const int death_delay = 25; // Delay in notes after which the player is hidden
	const float join_delay = 7.0f; // Time to select track/difficulty when joining mid-game
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
	float y2a(float y) { return time2a(past - y / timescale * (future - past)); }
	const double maxTolerance = 0.15; // Maximum error in seconds
	int getNextBigStreak(int prev) { return prev + 10; }

	/// Get an accuracy value [0, 1] for the error offset (in seconds)
	double accuracy(double error) { return 1.0 - (std::abs(error) / maxTolerance); };

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
			if (ac > .90) return "Perfect!";
			if (ac > .80) return "Excellent!-";
			if (ac > .70) return "Great!-";
			if (ac > .60) return " Good!- ";
			if (ac > .40) return "  OK!-  ";
			if (ac > .20) return "Late!-";
			return "Way off!";
		} else {
			if (ac > .90) return "Perfect!";
			if (ac > .80) return "-Excellent!";
			if (ac > .70) return "-Great!";
			if (ac > .60) return " -Good! ";
			if (ac > .40) return "  -OK!  ";
			if (ac > .20) return "-Early!";
			return "Way off!";
		}
	}

	struct lessEnd {
		bool operator()(const DanceNote& left, const DanceNote& right) {
			return left.note.end < right.note.end;
		}
	};
}


/// Constructor
DanceGraph::DanceGraph(Audio& audio, Song const& song):
  InstrumentGraph(audio, song, input::DANCEPAD),
  m_level(BEGINNER),
  m_beat(getThemePath("dancebeat.svg")),
  m_arrows(getThemePath("arrows.svg")),
  m_arrows_cursor(getThemePath("arrows_cursor.svg")),
  m_arrows_hold(getThemePath("arrows_hold.svg")),
  m_mine(getThemePath("mine.svg")),
  m_flow_direction(1)
{
	// Initialize some arrays
	for(size_t i = 0; i < max_panels; i++) {
		m_activeNotes[i] = m_notes.end();
		m_pressed[i] = false;
		m_pressed_anim[i] = AnimValue(0.0, 4.0);
		m_arrow_map[i] = -1;
	}

	if(m_song.danceTracks.empty())
		throw std::runtime_error("Could not find any dance tracks.");

	gameMode(0); // Get an initial game mode and notes for it
}

/// Attempt to select next/previous game mode
void DanceGraph::gameMode(int direction) {
	// Position mappings for panels
	static const int mapping4[max_panels] = {0, 1, 2, 3,-1,-1,-1,-1,-1,-1};
	static const int mapping5[max_panels] = {0, 1, 3, 4, 2,-1,-1,-1,-1,-1};
	static const int mapping6[max_panels] = {0, 2, 3, 5, 1, 4,-1,-1,-1,-1};
	static const int mapping7[max_panels] = {0, 2, 4, 6, 1, 5, 3,-1,-1,-1};
	static const int mapping8[max_panels] = {0, 3, 4, 7, 1, 6, 2, 5,-1,-1};
	static const int mapping10[max_panels]= {0, 3, 4, 7, 1, 6, 2, 5,-1,-1};
	// Cycling
	if (direction == 0) {
		m_curTrackIt = m_song.danceTracks.find("dance-single");
		if (m_curTrackIt == m_song.danceTracks.end())
			m_curTrackIt = m_song.danceTracks.begin();
	} else if (direction > 0) {
		m_curTrackIt++;
		if (m_curTrackIt == m_song.danceTracks.end()) m_curTrackIt = m_song.danceTracks.begin();
	} else if (direction < 0) {
		if (m_curTrackIt == m_song.danceTracks.begin()) m_curTrackIt = (--m_song.danceTracks.end());
		else m_curTrackIt--;
	}
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

	difficultyDelta(0); // Construct new notes
}

/// Are we alive?
bool DanceGraph::dead() const {
	return m_jointime != m_jointime || m_dead >= death_delay;
}

/// Get the difficulty as displayable string
std::string DanceGraph::getDifficultyString() const {
	std::string ret = diffv[m_level];
	if (m_input.isKeyboard()) ret += " (kbd)";
	return ret;
}

/// Attempt to change the difficulty by a step
void DanceGraph::difficultyDelta(int delta) {
	int newLevel = m_level + delta;
	if(newLevel >= DIFFICULTYCOUNT || newLevel < 0) return; // Out of bounds
	DanceTracks::const_iterator it = m_song.danceTracks.find(m_gamingMode);
	if(it->second.find((DanceDifficulty)newLevel) != it->second.end())
		difficulty((DanceDifficulty)newLevel);
	else
		difficultyDelta(delta + (delta < 0 ? -1 : 1));
}

/// Select a difficulty and construct DanceNotes and score normalizer for it
void DanceGraph::difficulty(DanceDifficulty level) {
	// TODO: error handling)
	m_notes.clear();
	DanceTrack const& track = m_song.danceTracks.find(m_gamingMode)->second.find(level)->second;
	for(Notes::const_iterator it = track.notes.begin(); it != track.notes.end(); it++)
		m_notes.push_back(DanceNote(*it));
	std::sort(m_notes.begin(), m_notes.end(), lessEnd()); // for engine's iterators
	m_notesIt = m_notes.begin();
	m_level = level;
	for(size_t i = 0; i < max_panels; i++) m_activeNotes[i] = m_notes.end();
	m_scoreFactor = 1;
	if(m_notes.size() != 0)
		m_scoreFactor = 10000.0 / (50 * m_notes.size()); // maxpoints / (notepoint * notes)
}

/// Handles input and some logic
void DanceGraph::engine() {
	double time = m_audio.getPosition();
	time -= config["audio/controller_delay"].f();
	for (Song::Stops::const_iterator it = m_song.stops.begin(), end = m_song.stops.end(); it != end; ++it) {
		if (it->first >= time) break;
		if (time < it->first + it->second) { time = it->first; break; } // Inside stop
		time -= it->second;
	}
	if (time < m_jointime) m_dead = 0; // Disable dead counting while joining
	bool difficulty_changed = false;
	// Handle all events
	for (input::Event ev; m_input.tryPoll(ev);) {
		m_dead = 0; // Keep alive
		if (m_jointime != m_jointime) { // Handle joining
			m_jointime = time < 0.0 ? -1.0 : time + join_delay;
			break;
		}
		// Difficulty / mode selection
		if (time < m_jointime && ev.type == input::Event::PRESS) {
			if (ev.pressed[STEP_UP]) difficultyDelta(1);
			else if (ev.pressed[STEP_DOWN]) difficultyDelta(-1);
			else if (ev.pressed[STEP_LEFT]) gameMode(-1);
			else if (ev.pressed[STEP_RIGHT]) gameMode(1);
			difficulty_changed = true;
		}
		// Gaming controls
		if (ev.type == input::Event::RELEASE) {
			m_pressed[ev.button] = false;
			dance(time, ev);
			m_pressed_anim[ev.button].setTarget(0.0);
		} else if (ev.type == input::Event::PRESS) {
			m_pressed[ev.button] = true;
			dance(time, ev);
			m_pressed_anim[ev.button].setValue(1.0);
		}
	}
	// Countdown to start
	handleCountdown(time, time < getNotesBeginTime() ? getNotesBeginTime() : m_jointime+1);

	// Notes gone by
	for (DanceNotes::iterator& it = m_notesIt; it != m_notes.end() && time > it->note.end + maxTolerance; it++) {
		if(!it->isHit) { // Missed
			if (it->note.type != Note::MINE) m_streak = 0;
		} else { // Hit, add score
			if(it->note.type != Note::MINE) m_score += it->score;
			if(!it->releaseTime) it->releaseTime = time;
		}
		++m_dead;
	}
	if (difficulty_changed) m_dead = 0; // if difficulty is changed, m_dead would get incorrect

	// Holding button when mine comes?
	for (DanceNotes::iterator it = m_notesIt; it != m_notes.end() && time <= it->note.begin + maxTolerance; it++) {
		if(!it->isHit && it->note.type == Note::MINE && m_pressed[it->note.note] &&
		  it->note.begin >= time - maxTolerance && it->note.end <= time + maxTolerance) {
			it->isHit = true;
			m_score -= points(0);
		}
	}

	// Check if a long streak goal has been reached
	if (m_streak >= getNextBigStreak(m_bigStreak)) {
		m_bigStreak = getNextBigStreak(m_bigStreak);
		m_popups.push_back(Popup(boost::lexical_cast<std::string>(unsigned(m_bigStreak)) + "\nStreak!",
		  glutil::Color(1.0f, 0.0, 0.0), 1.0, m_popupText.get()));
	}
}

/// Handles scoring and such
void DanceGraph::dance(double time, input::Event const& ev) {
	// Handle release events
	if(ev.type == input::Event::RELEASE) {
		DanceNotes::iterator it = m_activeNotes[ev.button];
		if(it != m_notes.end()) {
			if(!it->releaseTime && it->note.end > time + maxTolerance) {
				it->releaseTime = time;
				it->score = 0;
				m_streak = 0;
			}
		}
		return;
	}

	// So it was a PRESS event
	for (DanceNotes::iterator it = m_notesIt; it != m_notes.end() && time <= it->note.end + maxTolerance; it++) {
		if(!it->isHit && std::abs(time - it->note.begin) <= maxTolerance && ev.button == it->note.note) {
			it->isHit = true;
			if (it->note.type != Note::MINE) {
				it->score = points(it->note.begin - time);
				it->error = it->note.begin - time;
				m_streak++;
				if (m_streak > m_longestStreak) m_longestStreak = m_streak;
			} else { // Mine!
				m_score -= points(0);
				m_streak = 0;
			}
			m_activeNotes[ev.button] = it;
			break;
		}
	}
}


namespace {
	const float arrowSize = 0.4f; // Half width of an arrow
	const float one_arrow_tex_w = 1.0 / 8.0; // Width of a single arrow in texture coordinates

	/// Create a symmetric vertex pair for arrow drawing
	void vertexPair(int arrow_i, float x, float y, float ty, float scale = 1.0f) {
		if (arrow_i < 0) return;
		glTexCoord2f(arrow_i * one_arrow_tex_w, ty); glVertex2f(x - arrowSize * scale, y);
		glTexCoord2f((arrow_i+1) * one_arrow_tex_w, ty); glVertex2f(x + arrowSize * scale, y);
	}

	glutil::Color& colorGlow(glutil::Color& c, double glow) {
		//c.a = std::sqrt(1.0 - glow);
		c.a = 1.0 - glow;
		c.r += glow *.5;
		c.g += glow *.5;
		c.b += glow *.5;
		return c;
	}
}

/// Draw a dance pad icon using the given texture
void DanceGraph::drawArrow(int arrow_i, Texture& tex, float x, float y, float scale, float ty1, float ty2) {
	glutil::PushMatrix pm;
	glTranslatef(x, y, 0.0f); // Move to place
	if (scale != 1.0f) glScalef(scale, scale, scale); // Scale if needed
	{
		UseTexture tblock(tex);
		glutil::Begin block(GL_TRIANGLE_STRIP);
		vertexPair(arrow_i, 0.0f, -arrowSize, ty1);
		vertexPair(arrow_i, 0.0f, arrowSize, ty2);
	}
}

/// Draw a mine note
void DanceGraph::drawMine(float x, float y, float rot, float scale) {
	glutil::PushMatrix pm;
	glTranslatef(x, y, 0.0f);
	if (scale != 1.0f) glScalef(scale, scale, scale);
	if (rot != 0.0f) glRotatef(rot, 0.0f, 0.0f, 1.0f);
	m_mine.draw();
}

/// Draws the dance graph
void DanceGraph::draw(double time) {
	for (Song::Stops::const_iterator it = m_song.stops.begin(), end = m_song.stops.end(); it != end; ++it) {
		if (it->first >= time) break;
		if (time < it->first + it->second) { time = it->first; break; } // Inside stop
		time -= it->second;
	}

	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	dimensions.screenTop().middle(m_cx.get()).stretch(m_width.get(), 1.0);
	double offsetX = 0.5 * (dimensions.x1() + dimensions.x2());
	double frac = 0.75;  // Adjustable: 1.0 means fully separated, 0.0 means fully attached

	{
		// Some matrix magic to get the viewport right
		glutil::PushMatrixMode pmm(GL_PROJECTION);
		glTranslatef(frac * 2.0 * offsetX, 0.0f, 0.0f);
		glutil::PushMatrixMode pmb(GL_MODELVIEW);
		glTranslatef((1.0 - frac) * offsetX, dimensions.y1(), 0.0f);
		float temp_s = dimensions.w() / 8.0f; // Allow for 8 pads to fit on a track
		glScalef(temp_s, temp_s, temp_s);

		// Draw the "neck" graph (beat lines)
		drawBeats(time);

		// Arrows on cursor
		glColor3f(1.0f, 1.0f, 1.0f);
		for (int arrow_i = 0; arrow_i < m_pads; ++arrow_i) {
			float x = panel2x(arrow_i);
			float y = time2y(0.0);
			float l = m_pressed_anim[arrow_i].get();
			float s = getScale() * (5.0 - l) / 5.0;
			drawArrow(arrow_i, m_arrows_cursor, x, y, s);
		}

		// Draw the notes
		for (DanceNotes::iterator it = m_notes.begin(); it != m_notes.end(); ++it) {
			if (it->note.end - time < past) continue;
			if (it->note.begin - time > future) continue;
			drawNote(*it, time); // Let's just do all the calculating in the sub, instead of passing them as a long list
		}
	}
	drawInfo(time, offsetX, dimensions); // Go draw some texts and other interface stuff
	glColor3f(1.0f, 1.0f, 1.0f);
}

void DanceGraph::drawBeats(double time) {
	UseTexture tex(m_beat);
	glutil::Begin block(GL_TRIANGLE_STRIP);
	float texCoord = 0.0f;
	float tBeg = 0.0f, tEnd;
	float w = 0.5 * m_pads * getScale();
	for (Song::Beats::const_iterator it = m_song.beats.begin(); it != m_song.beats.end() && tBeg < future; ++it, texCoord += texCoordStep, tBeg = tEnd) {
		tEnd = *it - time;
		//if (tEnd < past) continue;
		/*if (tEnd > future) {
			// Crop the end off
			texCoord -= texCoordStep * (tEnd - future) / (tEnd - tBeg);
			tEnd = future;
		}*/
		glutil::Color c(1.0f, 1.0f, 1.0f, time2a(tEnd));
		glNormal3f(0.0f, 1.0f, 0.0f); glTexCoord2f(0.0f, texCoord); glVertex2f(-w, time2y(tEnd));
		glNormal3f(0.0f, 1.0f, 0.0f); glTexCoord2f(1.0f, texCoord); glVertex2f(w, time2y(tEnd));
	}
}

/// Draws a single note (or hold)
void DanceGraph::drawNote(DanceNote& note, double time) {
	float tBeg = note.note.begin - time;
	float tEnd = note.note.end - time;
	int arrow_i = note.note.note;
	bool mine = note.note.type == Note::MINE;
	float x = panel2x(arrow_i);
	float s = getScale();
	float yBeg = time2y(tBeg);
	float yEnd = time2y(tEnd);
	glutil::Color c(1.0f, 1.0f, 1.0f);

	// Did we hit it?
	if (note.isHit && (note.releaseTime > 0.0 || std::abs(tEnd) < maxTolerance) && note.hitAnim.getTarget() == 0.0) {
		if (mine) note.hitAnim.setRate(1.0);
		note.hitAnim.setTarget(1.0, false);
	}
	double glow = note.hitAnim.get();

	if (yEnd - yBeg > arrowSize) {
		// Draw holds
		glColor4fv(c);
		if (note.isHit && note.releaseTime <= 0) { // The note is being held down
			yBeg = std::max(time2y(0.0), yBeg);
			yEnd = std::max(time2y(0.0), yEnd);
			glColor3f(1.0f, 1.0f, 1.0f);
		}
		if (note.releaseTime > 0) yBeg = time2y(note.releaseTime - time); // Oh noes, it got released!
		if (yEnd - yBeg > 0) {
			UseTexture tblock(m_arrows_hold);
			glutil::Begin block(GL_TRIANGLE_STRIP);
			// Draw end
			vertexPair(arrow_i, x, yEnd, 1.0f, s);
			float yMid = std::max(yEnd-arrowSize, yBeg+arrowSize);
			vertexPair(arrow_i, x, yMid, 2.0f/3.0f, s);
			// Draw middle
			vertexPair(arrow_i, x, yBeg+arrowSize, 1.0f/3.0f, s);
		}
		// Draw begin
		if (note.isHit && tEnd < 0.1) {
			glColor4fv(colorGlow(c,glow));
			s += glow;
		}
		drawArrow(arrow_i, m_arrows_hold, x, yBeg, s, 0.0f, 1.0f/3.0f);
	} else {
		// Draw short note
		if (mine) { // Mines need special handling
			c.a = 1.0 - glow; glColor4fv(c);
			s = getScale() * 0.8f + glow * 0.5f;
			float rot = int(time*360 * (note.isHit ? 2.0 : 1.0) ) % 360; // They rotate!
			if (note.isHit) yBeg = time2y(0.0);
			drawMine(x, yBeg, rot, s);
		} else { // Regular arrows
			s += glow;
			glColor4fv(colorGlow(c, glow));
			drawArrow(arrow_i, m_arrows, x, yBeg, s);
		}
	}
	// Draw a text telling how well we hit
	if (!mine && note.isHit) {
		std::string text;
		if (note.releaseTime <= 0.0 && tBeg < tEnd) { // Is being held down and is a hold note
			text = "HOLD";
			glow = 1.0;
		} else if (glow > 0.0) { // Released already, display rank
			text = note.score ? getRank(note.error) : "FAIL!";
		}
		if (!text.empty()) {
			glColor3f(1.0f, 1.0f, 1.0f);
			double sc = getScale() * 1.2 * arrowSize * (1.0 + glow);
			m_popupText->render(text);
			m_popupText->dimensions().middle(x).center(time2y(0.0)).stretch(sc, sc/2.0);
			m_popupText->draw();
		}
	}
}

/// Draw popups and other info texts
void DanceGraph::drawInfo(double time, double offsetX, Dimensions dimensions) {
	// Draw info
	if (time < m_jointime) {
		m_text.dimensions.screenBottom(-0.075).middle(-0.09 + offsetX);
		m_text.draw("^ " + getDifficultyString() + " v");
		m_text.dimensions.screenBottom(-0.050).middle(-0.09 + offsetX);
		m_text.draw("< " + getTrack() + " >");
	} else { // Draw scores
		m_text.dimensions.screenBottom(-0.35).middle(0.32 * dimensions.w() + offsetX);
		m_text.draw(boost::lexical_cast<std::string>(unsigned(getScore())));
		m_text.dimensions.screenBottom(-0.32).middle(0.32 * dimensions.w() + offsetX);
		m_text.draw(boost::lexical_cast<std::string>(unsigned(m_streak)) + "/"
		  + boost::lexical_cast<std::string>(unsigned(m_longestStreak)));
	}
	drawPopups(offsetX);
}
