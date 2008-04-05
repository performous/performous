#include <video.h>
#include <cmath>

#ifdef USE_FFMPEG_VIDEO
Video::Video(std::string const& _videoFile): m_mpeg(true, false, _videoFile), m_lastTime(), m_alpha()
{
}
#else
Video::Video(std::string const&) {}
#endif

Video::~Video() {
}

void Video::render(double time, double w, double h) {
#ifdef USE_FFMPEG_VIDEO
	VideoFrame& fr = m_videoFrame;
	// Time to switch frame?
	if (!fr.data.empty() && time >= fr.timestamp) {
		m_surface.reset(new Surface(fr.width, fr.height, Surface::RGB, &fr.data[0]));
		fr.data.clear();
		m_surfaceTime = fr.timestamp;
	}
	if (m_surface) {
		double tdist = std::abs(m_surfaceTime - time);
		m_alpha += (tdist < 0.2 ? 0.02f : -0.02f);
		if (m_alpha <= 0.0f) m_alpha = 0.0f;
		else {
			if (m_alpha > 1.2f) m_alpha = 1.2f;
			if (m_alpha < 1.0f) glColor4f(1.0f, 1.0f, 1.0f, m_alpha);
			m_surface->draw();
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
	if (time < m_lastTime) m_mpeg.seek(time);
	m_lastTime = time;
	// Preload the next future frame
	if (fr.data.empty()) while (m_mpeg.videoQueue.tryPop(fr) && fr.timestamp < time);
#else
	(void)time; (void)w; (void)h;
#endif
}

