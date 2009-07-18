#include "guitargraph.hh"

GuitarGraph::GuitarGraph(Song const& song): m_song(song), m_neck("guitarneck.svg") {}

void GuitarGraph::draw(double time) {
	if (m_song.tracks.empty()) return; // Can't render without tracks (FIXME: actually screen_song should choose a track for us)
	glutil::PushMatrix pmb;
	glTranslatef(0.0f, 0.3f, 0.0f);
	glRotatef(80.0f, 1.0f, 0.0f, 0.0f);
	glScalef(0.1f, 0.1f, 0.1f);
	float ts = 12.0f;
	float future = 3.0f;
	{
		UseTexture tex(m_neck);
		glutil::Begin block(GL_QUADS);
		glTexCoord2f(0.0f, -(time + future)); glColor4f(1.0f, 1.0f, 1.0f, 0.0f); glVertex2f(-2.5f, -future * ts);
		glTexCoord2f(1.0f, -(time + future)); glColor4f(1.0f, 1.0f, 1.0f, 0.0f); glVertex2f(2.5f, -future * ts);
		glTexCoord2f(1.0f, -(time + 0.0f)); glColor4f(1.0f, 1.0f, 1.0f, 1.0f); glVertex2f(2.5f, 0.0f * ts);
		glTexCoord2f(0.0f, -(time + 0.0f)); glColor4f(1.0f, 1.0f, 1.0f, 1.0f); glVertex2f(-2.5f, 0.0f * ts);
		glColor3f(1.0f, 1.0f, 1.0f);
	}
	enum Difficulty {
		DIFFICULTY_SUPAEASY,
		DIFFICULTY_EASY,
		DIFFICULTY_MEDIUM,
		DIFFICULTY_AMAZING,
		DIFFICULTYCOUNT
	} level = DIFFICULTY_MEDIUM;

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
		NoteMap::const_iterator it = nm.find(basepitch + fret);
		if (it == nm.end()) continue;
		Durations const& durs = it->second;
		for (Durations::const_iterator it2 = durs.begin(); it2 != durs.end(); ++it2) {
			glutil::Begin block(GL_QUADS);
			float tBeg = -(time - it2->begin);
			float tEnd = -(time - it2->end);
			if (tBeg > future) break;
			float x = -2.0f + fret;
			float w = 0.25f;
			glutil::Color c = fretColors[fret];
			c.a = 1.0f - tEnd / future; glColor4fv(c);
			glVertex2f(x - w, -tEnd * ts);
			glVertex2f(x + w, -tEnd * ts);
			c.a = 1.0f - tBeg / future; glColor4fv(c);
			glVertex2f(x + w, -tBeg * ts);
			glVertex2f(x - w, -tBeg * ts);
		}
	}
}
