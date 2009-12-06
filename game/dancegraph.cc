#include "dancegraph.hh"

#include "fs.hh"
#include "notes.hh"
#include "surface.hh"
#include <boost/lexical_cast.hpp>
#include <stdexcept>

namespace {
	const std::string diffv[] = { "Beginner", "Easy", "Medium", "Hard", "Challenge" };
	const float past = -0.4f;
	const float future = 2.0f;
	const float timescale = 7.0f;
	// Note: t is difference from playback time so it must be in range [past, future]
	float time2y(float t) { return timescale * (t - past) / (future - past); }
	float time2a(float t) {
		float a = clamp(1.0 - t / future); // Note: we want 1.0 alpha already at zero t.
		return std::pow(a, 0.8f); // Nicer curve
	}
	float y2a(float y) { return time2a(past - y / timescale * (future - past)); }
	const double maxTolerance = 0.15;
	
	// TODO: Use this or something else?
	double points(double error) {
		error = (error < 0.0) ? -error : error;
		double score = 0.0;
		if (error < maxTolerance) score += 15;
		if (error < maxTolerance / 2) score += 15;
		if (error < maxTolerance / 4) score += 15;
		if (error < maxTolerance / 6) score += 5;
		return score;
	}
}


/// Constructor
DanceGraph::DanceGraph(Audio& audio, Song const& song):
  m_level(BEGINNER),
  m_audio(audio),
  m_song(song),
  m_input(input::GUITAR), // TODO: to be replaced by DANCEPAD
  m_arrow(getThemePath("arrow.svg")),
  m_mine(getThemePath("mine.svg")),
  m_arrow_hold(getThemePath("arrow_hold.svg")),
  m_cx(0.0, 0.2),
  m_width(0.5, 0.4),
  m_stream(),
  m_dead(1000),
  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
  m_correctness(0.0, 5.0),
  m_flow_direction(1),
  m_score(),
  m_scoreFactor(1),
  m_streak(),
  m_longestStreak(),
  m_gamingMode("dance-single")
{
	m_arrow.dimensions.middle().center();
	
	for(size_t i = 0; i < 4; i++) m_pressed[i] = false;
	for(size_t i = 0; i < 4; i++) m_pressed_anim[i] = AnimValue(0.0, 4.0);
	
	DanceTracks::const_iterator it = m_song.danceTracks.find(m_gamingMode);
	if(it == m_song.danceTracks.end())
		throw std::runtime_error("Could not find any dance tracks.");
	difficultyDelta(0); // hack to get initial level
	
}

std::string DanceGraph::getDifficultyString() const { return diffv[m_level]; }

void DanceGraph::difficultyDelta(int delta) {
	int newLevel = m_level + delta;
	std::cout << "difficultyDelta called with " << delta << " (newLevel = " << newLevel << ")" << std::endl;
	if(newLevel >= DIFFICULTYCOUNT || newLevel < 0)
		return;
	DanceTracks::const_iterator it = m_song.danceTracks.find(m_gamingMode);
	if(it->second.find((DanceDifficulty)newLevel) != it->second.end())
		difficulty((DanceDifficulty)newLevel);
	else
		difficultyDelta(delta + (delta < 0 ? -1 : 1));
}

void DanceGraph::difficulty(DanceDifficulty level) {
	// TODO: error handling)
	m_notes.clear();
	DanceTrack const& track = m_song.danceTracks.find(m_gamingMode)->second.find(level)->second;
	for(Notes::const_iterator it = track.notes.begin(); it != track.notes.end(); it++)
		m_notes.push_back(DanceNote(*it));
	m_notesIt = m_notes.begin();
//	std::cout << "Difficulty set to: " << level << std::endl;
	m_level = level;	
}

/// Handles input
void DanceGraph::engine() {
	double time = m_audio.getPosition();
	time -= config["audio/controller_delay"].f();

	// Handle all events
	for (input::Event ev; m_input.tryPoll(ev);) {
		m_dead = false;
		if(ev.button < 0 || ev.button > 3)
			continue;
		if (time < -0.5) {
			if (ev.type == input::Event::PRESS) {
				if (ev.pressed[STEP_UP]) difficultyDelta(1);
				else if (ev.pressed[STEP_DOWN]) difficultyDelta(-1);
			}
		}
		if (ev.type == input::Event::RELEASE) {
			m_pressed[ev.button] = false;
			dance(time, ev);
		}
		else if (ev.type == input::Event::PRESS) {
			m_pressed[ev.button] = true;
			dance(time, ev);
			m_pressed_anim[ev.button].setValue(1.0);
			m_pressed_anim[ev.button].setTarget(0.0);
		}
	}
	
	/**
	 * Idea in the usage of the iterator:
	 * Here it is used as reference to avoid needless iterations of the past notes during later calls of engine().
	 * Compare to the usage in the dance() function!
	 * Iterator is first initialized in the difficulty setting..
	**/
/*	for (DanceNotes::iterator& it = m_notesIt; it != m_notes.end() && it->note.begin < time - maxTolerance; it++) {
		if(!it->isHit)
			std::cout << "Missed note at time " << time
			<< "(note timing " << it->note.begin << ")" << std::endl;
	}*/

}


/// Handles scoring and such
void DanceGraph::dance(double time, input::Event const& ev) {
	if(ev.type == input::Event::RELEASE) {
		// (at least) initial hack for testing hold end..
		for (DanceNotes::iterator it = m_notesIt; it != m_notes.end() && it->note.begin <= time + maxTolerance; it++) {
			if(it->isHit && it->note.end - it->note.begin > 0.5) {
				it->releaseTime = time;
				it->isHit = false;
			}
		}
		return;
	}

	std::cout << "Pressed button " << ev.button << " at time " << time << std::endl;
	/**
	 * Idea behind the usage of the iterator:
	 * Here it is copied to a local variable, because jumping back to the first accepted time (within tolerance)
	 * is necessary in order to recognize notes closely timed notes.
	**/
	for (DanceNotes::iterator it = m_notesIt; it != m_notes.end() && it->note.begin <= time + maxTolerance; it++) {
		if(!it->isHit && ev.button == it->note.note) {
			// following clause needless?
			if(!(it->note.begin >= time - maxTolerance))
				std::cout << "Missed note. Timing: " << it->note.begin << "; Time: " << time
				<< "(difference " << (it->note.begin - time) << ")" << std::endl;
			it->isHit = true;
			double p = points(it->note.begin - time);
			std::cout << "Hit note " << ev.button << " at time " << time
			  << "; " << p << " points." << std::endl;
			it->score = p;
			m_score += p;
		}
	}
}


namespace {
	const float arrowScale = 0.6f;
	const float arrowRotations[4] = { 270.0f, 180.0f, 0.0f, 90.0f };
	const float holdWidth = 0.3;
	
	void vertexPair(float x, float y, float ty) {
		glTexCoord2f(0.0f, ty); glVertex2f(x - holdWidth, y);
		glTexCoord2f(1.0f, ty); glVertex2f(x + holdWidth, y);
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

glutil::Color const& DanceGraph::color(int arrow_i) const {
	static glutil::Color arrowColors[4] = {
		glutil::Color(0.0f, 0.9f, 0.0f),
		glutil::Color(0.9f, 0.0f, 0.0f),
		glutil::Color(0.9f, 0.9f, 0.0f),
		glutil::Color(0.0f, 0.0f, 0.9f),
	};
	if (arrow_i < 0 || arrow_i > 3) throw std::logic_error("Invalid arrow index in DanceGraph::getColor");
	return arrowColors[arrow_i];
}

void DanceGraph::drawArrow(int arrow_i, float x, float y, float scale) {
	glTranslatef(x, y, 0.0f);
	glRotatef(arrowRotations[arrow_i], 0.0f, 0.0f, 1.0f);
	if (scale != 1.0) glScalef(scale, scale, scale);
	m_arrow.draw();
	if (scale != 1.0) glScalef(1.0/scale, 1.0/scale, 1.0/scale);
	glRotatef(-arrowRotations[arrow_i], 0.0f, 0.0f, 1.0f);
	glTranslatef(-x, -y, 0.0f);
}

void DanceGraph::drawMine(float x, float y, float rot, float scale) {
	glTranslatef(x, y, 0.0f);
	glRotatef(rot, 0.0f, 0.0f, 1.0f);
	if (scale != 1.0) glScalef(scale, scale, scale);
	m_mine.draw();
	if (scale != 1.0) glScalef(1.0/scale, 1.0/scale, 1.0/scale);
	glRotatef(-rot, 0.0f, 0.0f, 1.0f);
	glTranslatef(-x, -y, 0.0f);
}

/// Draws the dance graph
void DanceGraph::draw(double time) {
	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	dimensions.screenTop().middle(m_cx.get()).stretch(m_width.get(), 1.0);
	double offsetX = 0.5 * (dimensions.x1() + dimensions.x2());
	double frac = 0.75;  // Adjustable: 1.0 means fully separated, 0.0 means fully attached
	// Draw scores
	if (time >= -0.5) {
		m_text.dimensions.screenBottom(-0.30).middle(0.32 * dimensions.w() + offsetX);
		m_text.draw(boost::lexical_cast<std::string>(unsigned(getScore())));
		m_text.dimensions.screenBottom(-0.27).middle(0.32 * dimensions.w() + offsetX);
		m_text.draw(boost::lexical_cast<std::string>(unsigned(m_streak)) + "/" 
		  + boost::lexical_cast<std::string>(unsigned(m_longestStreak)));
	} else {
		m_text.dimensions.screenBottom(-0.041).middle(-0.09 + offsetX);
		m_text.draw(getDifficultyString());
		m_text.dimensions.screenBottom(-0.015).middle(-0.09 + offsetX);
		m_text.draw(getGameMode());
	}
	glutil::PushMatrixMode pmm(GL_PROJECTION);
	glTranslatef(frac * 2.0 * offsetX, 0.0f, 0.0f);
	glutil::PushMatrixMode pmb(GL_MODELVIEW);
	glTranslatef((1.0 - frac) * offsetX, dimensions.y1(), 0.0f);
	{ float s = dimensions.w() / 5.0f; glScalef(s, s, s); }

	// Arrows on cursor
	for (int arrow_i = 0; arrow_i < 4; ++arrow_i) {
		float x = -1.5f + arrow_i;
		float l = m_pressed_anim[arrow_i].get();
		float s = (5.0 - l) / 5.0;
		glutil::Color c = color(arrow_i);
		c.r += l; c.g += l; c.b +=l;
		glColor4fv(c);
		drawArrow(arrow_i, x, time2y(0.0), arrowScale * s);
	}

	// Draw the notes
	for (DanceNotes::iterator it = m_notes.begin(); it != m_notes.end(); ++it) {
		if (it->note.end - time < past) continue;
		if (it->note.begin - time > future) break;
		drawNote(*it, time);
	}

	// To test arrow coordinate positioning
	//for (float i = past; i <= future; i+=0.2) {
		//std::cout << i << ": " << time2y(i) << std::endl;
		//drawArrow(1, 0, time2y(i), 0.6);
	//}

	glColor3f(1.0f, 1.0f, 1.0f);
}

/// Draws a single note (or hold)
void DanceGraph::drawNote(DanceNote& note, double time) {
	float tBeg = note.note.begin - time;
	float tEnd = note.note.end - time;
	int arrow_i = note.note.note;
	bool mine = note.note.type == Note::MINE;
	float x = -1.5f + arrow_i;
	float s = arrowScale;
	float yBeg = time2y(tBeg);
	float yEnd = time2y(tEnd);
	glutil::Color c = color(arrow_i);
	
	if (note.isHit && std::abs(tEnd) < maxTolerance) {
		if (mine) note.hitAnim.setRate(1.0);
		note.hitAnim.setTarget(1.0, false);
	}
	double glow = note.hitAnim.get();
	
	if (tEnd - tBeg > 0.1) {
		// Draw holds
		glColor4fv(c);
		if (note.isHit && note.releaseTime <= 0) {
			yBeg = std::max(time2y(0.0), yBeg);
			yEnd = std::max(time2y(0.0), yEnd);
			glColor3f(1.0f, 1.0f, 1.0f);
			// Hack to test hold releasing
			//if (time > note.note.begin + 1) note.releaseTime = time;
		}
		if (note.releaseTime > 0) yBeg = time2y(note.releaseTime - time);
		{ // Scope block for Begin and UseTexture
			UseTexture tblock(m_arrow_hold);
			glutil::Begin block(GL_TRIANGLE_STRIP);
			vertexPair(x, yEnd, 1.0f);
			vertexPair(x, yBeg, 0.0f);
		}
		// Draw begin
		if (note.isHit && tEnd < 0.1) {
			glColor4fv(colorGlow(c,glow));
			s = arrowScale + glow;
		}
		drawArrow(arrow_i, x, yBeg, s);
	} else {
		// Draw short note
		if (mine) {
			c.a = 1.0 - glow; glColor4fv(c);
			s += glow * 0.5;
			float rot = int(time*360 * (note.isHit ? 2.0 : 1.0) ) % 360;
			if (note.isHit) yBeg = time2y(0.0);
			drawMine(x, yBeg, rot, s);
		} else {
			s = arrowScale + glow;
			glColor4fv(colorGlow(c, glow));
			drawArrow(arrow_i, x, yBeg, s);
		}
	}
}
