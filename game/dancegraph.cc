#include "dancegraph.hh"
#include "fs.hh"


namespace {
	const float past = -0.2f;
	const float future = 1.8f;
	const float timescale = 25.0f;
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
		double score = 0.0;
		if (error < maxTolerance) score += 15;
		if (error < 0.1) score += 15;
		if (error < 0.05) score += 15;
		if (error < 0.03) score += 5;
		return score;
	}
}


/// Constructor
DanceGraph::DanceGraph(Audio& audio, Song const& song):
  m_audio(audio),
  m_song(song),
  //m_input(input::DANCEPAD),
  m_arrow(getThemePath("arrow.svg")),
  m_cx(0.0, 0.2),
  m_width(0.5, 0.4),
  m_stream(),
  m_dead(1000),
  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
  m_correctness(0.0, 5.0),
  m_flow_direction(1),
  m_score(),
  m_scoreFactor(),
  m_streak(),
  m_longestStreak()
{
	//TODO
	m_arrow.dimensions.middle().center();
	
}


/// Handles input
void DanceGraph::engine() {
	//TODO
	
}


/// Handles scoring and such
void DanceGraph::dance(double time, input::Event const& ev) {
	//TODO
	
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
std::cout << "m" << dimensions.x1() << " " << dimensions.x2() << std::endl;
std::cout << dimensions.y1() << " " << dimensions.y2() << std::endl;
	glutil::PushMatrixMode pmm(GL_PROJECTION);
	glTranslatef(frac * 2.0 * offsetX, 0.0f, 0.0f);
	glutil::PushMatrixMode pmb(GL_MODELVIEW);
	glTranslatef((1.0 - frac) * offsetX, dimensions.y1(), 0.0f);
	//glTranslatef((1.0 - frac) * offsetX, 0.0f, 0.0f);
	{ float s = dimensions.w() / 5.0f; glScalef(s, s, s); }
	// Draw the notes
	// TODO: iteration needs rewrite to the dancenotes structure
	//for (Chords::const_iterator it = m_chords.begin(); it != m_chords.end(); ++it) {
		/*
		float tBeg = it->begin - time;
		float tEnd = it->end - time;
		if (tEnd < past) continue;
		if (tBeg > future) break;
		for (int arrow_i = 0; arrow_i < 5; ++arrow_i) {
			//if (!it->fret[arrow_i]) continue;
			if (tEnd > future) tEnd = future;
			unsigned event = m_notes[it->dur[fret]];
			float glow = 0.0f;
			if (event > 0) glow = m_events[event - 1].glow.get();

			glutil::Color c = color(arrow_i);
			c.r += glow;
			c.g += glow;
			c.b += glow;
			drawNote(fret, c, tBeg, tEnd);
		}
		*/
	//}
	// Arrows on cursor
	// TODO: suitable effect for pressing the arrows?
	// TODO: effect possibilities: zooming, whitening, external glow
	for (int arrow_i = 0; arrow_i < 4; ++arrow_i) {
		float x = -1.5f + arrow_i;
		glColor4fv(color(arrow_i));
		drawArrow(arrow_i, x, time2y(0.0), 0.6);
	}
	glColor3f(1.0f, 1.0f, 1.0f);	
}


/// Draws a single note (or hold)
void DanceGraph::drawNote(int arrow_i, glutil::Color c, float tBeg, float tEnd) {
	float x = -1.5f + arrow_i;
	float yBeg = time2y(tBeg);
	float yEnd = time2y(tEnd);
	c.a = time2a(tBeg); glColor4fv(c);
	drawArrow(arrow_i, x, time2y(tBeg));
}
