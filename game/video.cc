#include "video.hh"

#include "util.hh"
#include <cmath>

bool Video::Fifo::tryPop(Bitmap& f) {
	std::unique_lock<std::mutex> l(m_mutex);
	if (m_queue.empty()) return false; // Nothing to deliver
	f = std::move(m_queue.front());
        m_readPosition = f.timestamp;
	m_queue.pop_front();
	m_cond.notify_all();
	return true;
}

void Video::Fifo::push(Bitmap&& f) {
	std::unique_lock<std::mutex> l(m_mutex);
	m_cond.wait(l, [this]{ return m_queue.size() < m_max; });
	m_queue.emplace_back(std::move(f));
}

void Video::Fifo::reset() {
	std::unique_lock<std::mutex> l(m_mutex);
	m_queue.clear();
	m_cond.notify_all();
}

Video::~Video() { m_quit = true; videoQueue.reset(); m_grabber.get(); }

Video::Video(fs::path const& _videoFile, double videoGap): m_mpeg(_videoFile, 0, nullptr, [this] (auto f) { videoQueue.push(std::move(f)); }), m_videoGap(videoGap), m_textureTime(), m_lastTime(), m_alpha(-0.5, 1.5) {
   
   m_grabber = std::async(std::launch::async, [this, file = _videoFile] {
	int errors = 0;
	while (!m_quit) {
		try {
			if (m_lastTime < videoQueue.position() - 1 || m_lastTime > videoQueue.position() + 5) {
				m_mpeg.seek(std::max(0.0, m_lastTime - 5.0));  // -5 to workaround ffmpeg inaccurate seeking
                                videoQueue.reset();
                                std::clog << "ffmpeg/debug: seeking " << m_lastTime << " curr " << videoQueue.position() << std::endl;
			}
			m_mpeg.handleOneFrame();
		} catch (FFmpeg::eof_error&) {
			videoQueue.push(Bitmap()); // EOF marker
			std::clog << "ffmpeg/debug: done loading " << file << std::endl;
		} catch (std::exception& e) {
			std::clog << "ffmpeg/error: " << file << ": " << e.what() << std::endl;
			if (++errors > 2) { std::clog << "ffmpeg/error: FFMPEG terminating due to multiple errors" << std::endl; break; }
		}
		errors = 0;
	}
   });
}

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
	if (fr.buf.empty()) while (videoQueue.tryPop(fr) && fr.timestamp < time) {};
	// Do a seek before next render, if required
	m_lastTime = time;
        std::clog << "ffmpeg/debug: got frame @ " << m_lastTime << std::endl;
}

void Video::render(double time) {
	time += m_videoGap;
	double tdist = std::abs(m_textureTime - time);
	m_alpha.setTarget(tdist < 0.4 ? 1.2f : -0.5f);
	double alpha = clamp(m_alpha.get());
	if (alpha == 0.0) return;
	ColorTrans c(Color::alpha(alpha));
	m_texture.draw();
}

