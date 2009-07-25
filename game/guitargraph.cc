#include "guitargraph.hh"

namespace {
	const float past = -0.3f;
	const float future = 3.0f;
	const float timescale = 80.0f;
	const float texCoordStep = -0.5f; // Two beat lines per neck texture => 0.5 tex units per beat
	// Note: t is difference from playback time so it must be in range [past, future]
	float time2y(float t) { return -timescale * (t - past) / (future - past); }
	float time2a(float t) { return 1.0f - t / future; } // Note: we want 1.0 alpha already at zero t.
}

GuitarGraph::GuitarGraph(Song const& song): m_song(song), m_neck("guitarneck.svg"), m_button("button.svg") {}

void GuitarGraph::draw(double time) {
	if (m_song.tracks.empty()) return; // Can't render without tracks (FIXME: actually screen_song should choose a track for us)
	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	dimensions.screenBottom().fixedWidth(0.5f);
	glutil::PushMatrix pmb;
	glTranslatef(0.5 * (dimensions.x1() + dimensions.x2()), dimensions.y2(), 0.0f);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	{ float s = dimensions.w() / 5.0f; glScalef(s, s, s); }
	// Draw the neck
	{
		UseTexture tex(m_neck);
		glutil::Begin block(GL_TRIANGLE_STRIP);
		float texCoord = 0.0f;
		float tBeg = 0.0f, tEnd;
		for (Song::Beats::const_iterator it = m_song.beats.begin(); it != m_song.beats.end() && tBeg < future; ++it, texCoord += texCoordStep, tBeg = tEnd) {
			tEnd = *it - time;
			//if (tEnd < past) continue;
			if (tEnd > future) {
				// Crop the end off
				texCoord -= texCoordStep * (tEnd - future) / (tEnd - tBeg);
				tEnd = future;
			}
			glColor4f(1.0f, 1.0f, 1.0f, time2a(tEnd));
			glTexCoord2f(0.0f, texCoord); glVertex2f(-2.5f, time2y(tEnd));
			glTexCoord2f(1.0f, texCoord); glVertex2f(2.5f, time2y(tEnd));
		}
	}
	enum Difficulty {
		DIFFICULTY_SUPAEASY,
		DIFFICULTY_EASY,
		DIFFICULTY_MEDIUM,
		DIFFICULTY_AMAZING,
		DIFFICULTYCOUNT
	} level = DIFFICULTY_AMAZING;

	int basepitch;	
	switch (level) {
		case DIFFICULTY_SUPAEASY: basepitch = 0x3c; break;
		case DIFFICULTY_EASY: basepitch = 0x48; break;
		case DIFFICULTY_MEDIUM: basepitch = 0x54; break;
		case DIFFICULTY_AMAZING: basepitch = 0x60; break;
		default: throw std::logic_error("Invalid difficulty level");
	}

	static glutil::Color fretColors[5] = {
		glutil::Color(0.0f, 1.0f, 0.0f),
		glutil::Color(1.0f, 0.0f, 0.0f),
		glutil::Color(1.0f, 1.0f, 0.0f),
		glutil::Color(0.0f, 0.0f, 1.0f),
		glutil::Color(1.0f, 0.5f, 0.0f)
	};
	// Draw the notes
	NoteMap const& nm = m_song.tracks.begin()->second;
	for (int fret = 0; fret < 5; ++fret) {
		float x = -2.0f + fret;
		float w = 0.5f;
		glutil::Color c = fretColors[fret];
		NoteMap::const_iterator it = nm.find(basepitch + fret);
		if (it == nm.end()) continue;
		Durations const& durs = it->second;
		for (Durations::const_iterator it2 = durs.begin(); it2 != durs.end(); ++it2) {
			float tBeg = it2->begin - time;
			float tEnd = it2->end - time;
			if (tEnd < past) continue;
			if (tBeg > future) break;
			float wEnd = 0.5f * w;
			if (tEnd > future) {
				// Crop the end off
				float f = (future - tBeg) / (tEnd - tBeg);
				wEnd = f * wEnd + (1.0f - f) * w; // Balanced average
				tEnd = future;
			}
			glutil::Begin block(GL_TRIANGLE_STRIP);
			c.a = time2a(tEnd); glColor4fv(c);
			glVertex2f(x - wEnd, time2y(tEnd));
			glVertex2f(x + wEnd, time2y(tEnd));
			c.a = time2a(tBeg); glColor4fv(c);
			glVertex2f(x - w, time2y(tBeg));
			glVertex2f(x + w, time2y(tBeg));
		}
	}
	glColor3f(0.0f, 0.0f, 0.0f);
	// Draw the cursor
	{	
		glutil::Begin block(GL_TRIANGLE_STRIP);
		glVertex2f(-2.5f, time2y(0.01f));
		glVertex2f(2.5f, time2y(0.01f));
		glVertex2f(-2.5f, time2y(-0.01f));
		glVertex2f(2.5f, time2y(-0.01f));
	}
	for (int fret = 0; fret < 5; ++fret) {
		float x = -2.0f + fret;
		glutil::Color c = fretColors[fret];
		if (true) {
			c.r *= 0.5f;
			c.g *= 0.5f;
			c.b *= 0.5f;
		}
		glColor4fv(c);
		m_button.dimensions.center(time2y(0.0)).middle(x);
		m_button.draw();
	}
	glColor3f(1.0f, 1.0f, 1.0f);
}

