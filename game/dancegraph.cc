#include "dancegraph.hh"
#include "fs.hh"

/// Constructor
DanceGraph::DanceGraph(Audio& audio, Song const& song):
  m_audio(audio),
  m_song(song),
  m_input(input::DANCEPAD),
  m_arrow("arrow.svg"),
  m_cx(0.0, 0.2),
  m_width(0.5, 0.4),
  m_stream(),
  m_dead(1000),
  m_text(getThemePath("sing_timetxt.svg"), config["graphic/text_lod"].f()),
  m_correctness(0.0, 5.0),
  m_flow_direction(1);
  m_score(),
  m_scoreFactor(),
  m_streak(),
  m_longestStreak()
{
	//TODO	
	
	
}


/// Handles input
void DanceGraph::engine() {
	//TODO	
	
}


/// Handles scoring and such
void DanceGraph::dance(double time, input::Event const& ev) {
	//TODO: 
	
}


glutil::Color const& DanceGraph::color(int arrow_i) const {
	static glutil::Color arrowColors[3] = {
		glutil::Color(0.0f, 0.9f, 0.0f),
		glutil::Color(0.9f, 0.0f, 0.0f),
		glutil::Color(0.9f, 0.9f, 0.0f),
		glutil::Color(0.0f, 0.0f, 0.9f),
	};
	if (arrow_i < 0 || arrow_i > 3) throw std::logic_error("Invalid arrow number in DanceGraph::getColor");
	return arrowColors[arrow_i];
}


/// Draws the dance graph
void DanceGraph::draw(double time) {
	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	dimensions.screenBottom().middle(m_cx.get()).fixedWidth(m_width.get());
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
	glutil::PushMatrixMode pmm(GL_PROJECTION);
	glTranslatef(frac * 2.0 * offsetX, 0.0f, 0.0f);
	glutil::PushMatrixMode pmb(GL_MODELVIEW);
	glTranslatef((1.0 - frac) * offsetX, dimensions.y2(), 0.0f);
	{ float s = dimensions.w() / 5.0f; glScalef(s, s, s); }
	// Draw the notes
	// TODO: iteration needs rewrite to the dancenotes structure
	//for (Chords::const_iterator it = m_chords.begin(); it != m_chords.end(); ++it) {
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
			drawNote(fret, c, tBeg, tEnd, whammy);
		}
	//}
	// Arrows on cursor
	// TODO: suitable effect for pressing the arrows?
	// TODO: effect possibilities: zooming, whitening, external glow
	for (int arrow_i = m_drums; arrow_i < 5; ++arrow_i) {
		float x = -2.0f + fret - 0.5f * m_drums;
		glColor4fv(color(arrow_i));
		m_arrow.dimensions.center(time2y(0.0)).middle(x);
		m_arrow.draw();
		float l = m_hit[fret + !m_drums].get();
	}
	glColor3f(1.0f, 1.0f, 1.0f);	
}


/// Draws a single note (or hold)
void DanceGraph::drawNote(int fret, glutil::Color, float tBeg, float tEnd) {
	//TODO
	
}
