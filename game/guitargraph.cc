#include "guitargraph.hh"

GuitarGraph::GuitarGraph(Song const& song): m_song(song), m_neck("guitarneck.svg") {}

void GuitarGraph::draw(double time) {
	UseTexture tex(m_neck);
	glutil::PushMatrix pmb;
	glTranslatef(0.0f, 0.3f, 0.0f);
	glRotatef(80.0f, 1.0f, 0.0f, 0.0f);
	glScalef(0.1f, 0.1f, 0.1f);
	float ts = 12.0f;
	float future = 3.0f;
	glutil::Begin block(GL_QUADS);
	glTexCoord2f(0.0f, -(time + future)); glColor4f(1.0f, 1.0f, 1.0f, 0.0f); glVertex2f(-2.5f, -future * ts);
	glTexCoord2f(1.0f, -(time + future)); glColor4f(1.0f, 1.0f, 1.0f, 0.0f); glVertex2f(2.5f, -future * ts);
	glTexCoord2f(1.0f, -(time + 0.0f)); glColor4f(1.0f, 1.0f, 1.0f, 1.0f); glVertex2f(2.5f, 0.0f * ts);
	glTexCoord2f(0.0f, -(time + 0.0f)); glColor4f(1.0f, 1.0f, 1.0f, 1.0f); glVertex2f(-2.5f, 0.0f * ts);
	glColor3f(1.0f, 1.0f, 1.0f);
}
