#include "guitargraph.hh"

namespace {
	const float past = -0.3f;
	const float future = 3.0f;
	const float timescale = 30.0f;
	// Note: t is difference from playback time so it must be in range [past, future]
	float time2y(float t) { return -timescale * (t - past) / (future - past); }
	float time2a(float t) { return 1.0f - t / future; } // Note: we want 1.0 alpha already at zero t.
}

GuitarGraph::GuitarGraph(Song const& song): m_song(song), m_neck("guitarneck.svg") {}

void GuitarGraph::draw(double time) {
	if (m_song.tracks.empty()) return; // Can't render without tracks (FIXME: actually screen_song should choose a track for us)
	Dimensions dimensions(1.0); // FIXME: bogus aspect ratio (is this fixable?)
	dimensions.screenBottom().fixedWidth(0.5f);
	glutil::PushMatrix pmb;
	glTranslatef(0.0f, dimensions.y2(), 0.0f);
	glRotatef(80.0f, 1.0f, 0.0f, 0.0f);
	{ float s = dimensions.w() / 5.0f; glScalef(s, s, s); }
	{
		UseTexture tex(m_neck);
		glutil::Begin block(GL_QUADS);
		glTexCoord2f(0.0f, -(time + future)); glColor4f(1.0f, 1.0f, 1.0f, 0.0f); glVertex2f(-2.5f, time2y(future));
		glTexCoord2f(1.0f, -(time + future)); glColor4f(1.0f, 1.0f, 1.0f, 0.0f); glVertex2f(2.5f, time2y(future));
		glTexCoord2f(1.0f, -(time + past)); glColor4f(1.0f, 1.0f, 1.0f, 1.0f); glVertex2f(2.5f, time2y(past));
		glTexCoord2f(0.0f, -(time + past)); glColor4f(1.0f, 1.0f, 1.0f, 1.0f); glVertex2f(-2.5f, time2y(past));
	}
	{	
		glutil::Begin block(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glVertex2f(-2.5f, time2y(0.01f));
		glVertex2f(2.5f, time2y(0.01f));
		glVertex2f(2.5f, time2y(-0.01f));
		glVertex2f(-2.5f, time2y(-0.01f));
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
	NoteMap const& nm = m_song.tracks.begin()->second;
	for (int fret = 0; fret < 5; ++fret) {
		float x = -2.0f + fret;
		float w = 0.5f;
		glutil::Color c = fretColors[fret];
		NoteMap::const_iterator it = nm.find(basepitch + fret);
		if (it == nm.end()) continue;
		Durations const& durs = it->second;
		glutil::Begin block(GL_QUADS);
		for (Durations::const_iterator it2 = durs.begin(); it2 != durs.end(); ++it2) {
			float tBeg = it2->begin - time;
			float tEnd = it2->end - time;
			if (tBeg > future) break;
			c.a = time2a(tBeg); glColor4fv(c);
			glVertex2f(x - 0.5f * w, time2y(tEnd));
			glVertex2f(x + 0.5f * w, time2y(tEnd));
			c.a = time2a(tBeg); glColor4fv(c);
			glVertex2f(x + w, time2y(tBeg));
			glVertex2f(x - w, time2y(tBeg));
		}
	}
	glColor3f(1.0f, 1.0f, 1.0f);
}
