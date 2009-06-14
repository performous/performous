#include "video.hh"
#include "configuration.hh"
#include <cmath>

Video::Video(std::string const& _videoFile): m_mpeg(true, false, _videoFile), m_surfaceTime(), m_lastTime(), m_alpha() {}

void Video::render(double time) {
	VideoFrame& fr = m_videoFrame;
	// Time to switch frame?
	if (!fr.data.empty() && time >= fr.timestamp) {
		m_surface.load(fr.width, fr.height, pix::RGB, &fr.data[0]);
		fr.data.clear();
		m_surfaceTime = fr.timestamp;
	}
	double tdist = std::abs(m_surfaceTime - time);
	m_alpha += (tdist < 0.4 ? 0.02f : -0.02f);
	if (m_alpha <= 0.0f) m_alpha = 0.0f;
	else {
		if (m_alpha > 1.2f) m_alpha = 1.2f;
		if (m_alpha < 1.0f) glColor4f(1.0f, 1.0f, 1.0f, m_alpha);
		m_surface.draw();
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
	// Preload the next future frame
	if (fr.data.empty()) while (m_mpeg.videoQueue.tryPop(fr) && fr.timestamp < time) {};
	// Do a seek before next render, if required
	if (time < m_lastTime - 0.4 || (!fr.data.empty() && time > fr.timestamp + 2.0)) {
		m_mpeg.seek(time - 0.3);
		fr.data.clear();
	}
	m_lastTime = time;
}

