#include "dancegraph.hh"

#include "fs.hh"
#include "notes.hh"
#include <boost/lexical_cast.hpp>
#include <stdexcept>

namespace {
	const float past = -0.2f;
	const float future = 1.8f;
	const float offsetY = 1.8f; // TODO: more clever way to do this?
	const float timescale = 6.0f;
	// Note: t is difference from playback time so it must be in range [past, future]
	float time2y(float t) { return offsetY + timescale * (t - past) / (future - past); }
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
	
	for(size_t i = 0; i < 4; i++) m_pressed[i] = AnimValue(0.0, 8.0);
	
	DanceTracks::const_iterator it = m_song.danceTracks.find(m_gamingMode);
	if(it == m_song.danceTracks.end())
		throw std::runtime_error("Could not find any dance tracks.");
	difficultyDelta(0); // hack to get initial level
	
}

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
			m_pressed[ev.button].setTarget(0.0, false);
		}
		else if (ev.type == input::Event::PRESS) {
			dance(time, ev);
			m_pressed[ev.button].setTarget(1.0, true);
		}
	}

	
}


/// Handles scoring and such
void DanceGraph::dance(double time, input::Event const& ev) {
	for (DanceNotes::iterator& it = m_notesIt; it != m_notes.end() && it->note.begin <= time + maxTolerance; ++it) {
		if(!it->isHit && ev.button == it->note.note) {
			it->isHit = true;
			double p = points(it->note.begin - time);
			it->score = p;
			m_score += p;
			it->hitAnim.setTarget(1.0, false);
		}
	}
}


glutil::Color const& DanceGraph::color(int arrow_i) const {
	static glutil::Color arrowColors[4] = {
		glutil::Color(0.0f, 0.9f, 0.0f),
		glutil::Color(0.9f, 0.0f, 0.0f),
		glutil::Color(0.9f, 0.9f, 0.0f),
		glutil::Color(0.0f, 0.0f, 0.9f),
	};
	if (arrow_i < 0 || arrow_i > 3) throw std::logic_error("Invalid arrow number in DanceGraph::getColor");
	return arrowColors[arrow_i];
}

namespace {
	const float arrowRotations[4] = { 270.0f, 180.0f, 0.0f, 90.0f };


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

/// Draws the dance graph
void DanceGraph::draw(double time) {
	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	//dimensions.screenTop().middle(m_cx.get()).fixedWidth(m_width.get());
	dimensions.screenCenter().middle(m_cx.get()).stretch(m_width.get(), 0.9);
	double offsetX = 0.5 * (dimensions.x1() + dimensions.x2());
	double frac = 0.75;  // Adjustable: 1.0 means fully separated, 0.0 means fully attached
	// Draw scores
	if (time >= -0.5) {
		m_text.dimensions.screenBottom(-0.30).middle(0.32 * dimensions.w() + offsetX);
		m_text.draw(boost::lexical_cast<std::string>(unsigned(getScore())));
		m_text.dimensions.screenBottom(-0.27).middle(0.32 * dimensions.w() + offsetX);
		m_text.draw(boost::lexical_cast<std::string>(unsigned(m_streak)) + "/" 
		  + boost::lexical_cast<std::string>(unsigned(m_longestStreak)));
	}
//std::cout << "m" << dimensions.x1() << " " << dimensions.x2() << std::endl;
//std::cout << dimensions.y1() << " " << dimensions.y2() << std::endl;
	glutil::PushMatrixMode pmm(GL_PROJECTION);
	glTranslatef(frac * 2.0 * offsetX, 0.0f, 0.0f);
	glutil::PushMatrixMode pmb(GL_MODELVIEW);
	glTranslatef((1.0 - frac) * offsetX, dimensions.y1(), 0.0f);
	//glTranslatef((1.0 - frac) * offsetX, 0.0f, 0.0f);
	{ float s = dimensions.w() / 5.0f; glScalef(s, s, s); }

	// Arrows on cursor
	// TODO: suitable effect for pressing the arrows?
	// TODO: effect possibilities: zooming, whitening, external glow
	for (int arrow_i = 0; arrow_i < 4; ++arrow_i) {
		float x = -1.5f + arrow_i;
		float l = m_pressed[arrow_i].get();
		float s = (5.0 - l) / 5.0;
		glutil::Color c = color(arrow_i);
		c.r += l; c.g += l; c.b +=l;
		glColor4fv(c);
		drawArrow(arrow_i, x, time2y(0.0), 0.6 * s);
	}

	// Draw the notes
	float tBeg, tEnd = 0.0f;
	for (DanceNotes::iterator it = m_notes.begin(); it != m_notes.end(); ++it) {
		tBeg = it->note.begin - time;
		tEnd = it->note.end - time;
		if (tEnd < past) continue;
		if (tBeg > future) break;
				
		int arrow_i = it->note.note;
		float x = -1.5f + arrow_i;
		float s = 0.6f;
		float yBeg = time2y(tBeg);
		float yEnd = time2y(tEnd);
		glutil::Color c = color(arrow_i);
		
		double glow = it->hitAnim.get();
		//c.a = std::sqrt(1.0 - glow);
		c.a = 1.0 - glow;
		c.r += glow *.5;
		c.g += glow *.5;
		c.b += glow *.5;
		s += glow;
		
		glColor4fv(c);
		drawArrow(arrow_i, x, yBeg, s);
		
		//drawNote(it->note.note, c, tBeg, tEnd);
	}

	// To test arrow coordinate positioning
//	for (float i = past; i < future; i+=0.2) {
//		std::cout << time2y(i) << std::endl;
//		drawArrow(1, 0, time2y(i), 0.6);
//	}

	glColor3f(1.0f, 1.0f, 1.0f);
}

// TODO: See if this is needed at all
/// Draws a single note (or hold)
void DanceGraph::drawNote(int arrow_i, glutil::Color c, float tBeg, float tEnd) {
	float x = -1.5f + arrow_i;
	float yBeg = time2y(tBeg);
	float yEnd = time2y(tEnd);
	//c.a = time2a(tBeg);
	glColor4fv(c);
	drawArrow(arrow_i, x, yBeg, 0.6);
}
