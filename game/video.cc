#include "video.hh"

#include "util.hh"
#include <cmath>

Video::Video(fs::path const& _videoFile, double videoGap): m_mpeg(_videoFile), m_videoGap(videoGap), m_textureTime(), m_lastTime(), m_alpha(-0.5f, 1.5f) {}

void Video::prepare(double time) {
	time += m_videoGap;
	Bitmap& fr = m_videoFrame;
	// Time to switch frame?
	if (!fr.buf.empty() && time >= fr.timestamp) {
		m_texture.load(fr);
		m_textureTime = fr.timestamp;
		fr.resize(0, 0);
	}
	// Preload the next future frame
	if (fr.buf.empty()) while (m_mpeg.videoQueue.tryPop(fr) && fr.timestamp < time) {};
	// Do a seek before next render, if required
	if (time < m_lastTime - 1.0 || (!fr.buf.empty() && time > fr.timestamp + 7.0)) {
		m_mpeg.seek(std::max(0.0, time - 5.0));  // -5 to workaround ffmpeg inaccurate seeking
		fr.buf.clear();
	}
	m_lastTime = time;
}

void Video::render(double time) {
	time += m_videoGap;
	double tdist = std::abs(m_textureTime - time);
	m_alpha.setTarget(tdist < 0.4 ? 1.2f : -0.5f);
	float alpha = clamp(m_alpha.get());
	if (alpha == 0.0f) return;
	ColorTrans c(Color::alpha(alpha));
	m_texture.draw();
}

