#include <video.h>

#ifdef USE_FFMPEG_VIDEO
Video::Video(std::string const& _videoFile): m_mpeg(true, false, _videoFile), m_lastTime()
{
	glGenTextures(1, &m_texture);
}
#else
Video::Video(std::string const&) {}
#endif

Video::~Video() {
#ifdef USE_FFMPEG_VIDEO
	glDeleteTextures(1, &m_texture);
#endif
}

void Video::render(double time, double w, double h) {
#ifdef USE_FFMPEG_VIDEO
	VideoFrame& fr = m_videoFrame;
	// Time to switch frame?
	if (!fr.data.empty() && time >= fr.timestamp) {
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_texture);
		glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, fr.width, fr.height, 0, GL_BGRA, GL_UNSIGNED_BYTE, &fr.data[0]);
		fr.data.clear();
	}
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_texture);
	double ar = double(fr.width) / fr.height / (w / h);
	glPushMatrix();
	glTranslatef(0.5 * w, 0.5 * h, 0.0);
	glScalef(w, h / ar, 1.0);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0); glVertex2f(-0.5, -0.5);
	glTexCoord2f(fr.width, 0.0); glVertex2f(0.5, -0.5);
	glTexCoord2f(fr.width, fr.height); glVertex2f(0.5, 0.5);
	glTexCoord2f(0.0, fr.height); glVertex2f(-0.5, 0.5);
	glEnd();
	glPopMatrix();
	if (time < m_lastTime) m_mpeg.seek(time);
	m_lastTime = time;
	// Preload the next future frame
	if (fr.data.empty()) while (m_mpeg.videoQueue.tryPop(fr) && fr.timestamp < time);
#else
	(void)time; (void)w; (void)h;
#endif
}

