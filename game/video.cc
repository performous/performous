#include "video.hh"

#include "ffmpeg.hh"
#include "log.hh"
#include "util.hh"
#include "graphic/color_trans.hh"

#include <cmath>

bool Video::tryPop(Bitmap& f, double timestamp) {
	std::unique_lock<std::mutex> l(m_mutex);

	// if timestamp is out of the queue's range, ask a seek
	// FIXME 4 should be linked to queue depth: we know the frame rate, thus how
	// many frames needed for an arbitrary duration
	m_seek_asked |= timestamp > m_readPosition + 4 || timestamp < m_readPosition;

	m_readPosition = timestamp;

	// if queue is not empty, we are sure will remove at least one element from it.
	// When seeking needed to wake up thread if waiting for rooms.
	if (!m_queue.empty() || m_seek_asked) m_cond.notify_all();

	if (m_seek_asked) return false;

	// discard outdated frames retaining only the most recent frame that is _before_ timestamp
	while (!m_queue.empty() && std::next(m_queue.begin()) != m_queue.end() && std::next(m_queue.begin())->timestamp < timestamp) m_queue.pop_front();

	if (m_queue.empty() || m_queue.front().timestamp > timestamp) return false; // Nothing to deliver

	f = std::move(m_queue.front());
	m_queue.pop_front();
	return true;
}

void Video::push(Bitmap&& f) {
	std::unique_lock<std::mutex> l(m_mutex);
	m_cond.wait(l, [this]{ return m_quit || m_seek_asked || m_queue.size() < m_max; });
	if (m_quit || m_seek_asked) return; // Drop frame when seek/quit asked
	m_queue.emplace_back(std::move(f));
}

Video::~Video() {
	{
		std::lock_guard<std::mutex> l(m_mutex);
		m_quit = true;
	}
	m_cond.notify_all();
	m_grabber.get();
}

Video::Video(fs::path const& _videoFile, double videoGap): m_videoGap(videoGap), m_textureTime(), m_alpha(-0.5f, 1.5f) {
	m_grabber = std::async(std::launch::async, [this, file = _videoFile] {
		try {
			auto ffmpeg = std::make_unique<VideoFFmpeg>(file, [this](auto f) { this->push(std::move(f)); });
			int errors = 0;
			std::unique_lock<std::mutex> l(m_mutex);
			while (!m_quit) {
				if (m_seek_asked) {
					m_seek_asked = false;

					auto seek_pos = m_readPosition;
					// discard all outdated frame. To avoid races between clean and push, clean and push are done in this thread.
					m_queue.clear();

					UnlockGuard<decltype(l)> unlocked(l);  // release lock during seek
					ffmpeg->seek(seek_pos);
					continue;
				}

				try {
					UnlockGuard<decltype(l)> unlocked(l);  // release lock during possibly blocking ffmpeg stuff
					ffmpeg->handleOneFrame();
					errors = 0;
				} catch (FFmpeg::Eof&) {
					{
						UnlockGuard<decltype(l)> unlocked(l);  // release lock for possibly blocking calls
						push(Bitmap());						   // EOF marker
						SpdLogger::debug(LogSystem::FFMPEG, "File={}, done loading.", file);
					}
					m_cond.wait(l, [this] { return m_quit || m_seek_asked; });
				} catch (std::exception& e) {
					UnlockGuard<decltype(l)> unlocked(l);  // release lock for possibly blocking calls
					SpdLogger::error(LogSystem::FFMPEG, "Error={}, reading file={}.", e.what(), file);
					if (++errors > 2) {
						SpdLogger::error(LogSystem::FFMPEG, "Terminating due to multiple errors.");
						break;
					}
				}
			}
		} catch (std::exception& e) {
			SpdLogger::error(LogSystem::FFMPEG, "Error={}, reading file={}.", e.what(), file);
		}
	});
}

void Video::prepare(double time) {
	if (std::isnan(time)) return;

	// shift video timestamp if gap is declared in song config
	time += m_videoGap;

	Bitmap videoFrame;
	if (tryPop(videoFrame, time) && !videoFrame.buf.empty()) {
		m_texture.load(videoFrame);
		m_textureTime = videoFrame.timestamp;
	}
}

void Video::render(Window& window, double time) {
	time += m_videoGap;
	double tdist = std::abs(m_textureTime - time);
	m_alpha.setTarget(tdist < 0.4 ? 1.2f : -0.5f);
	float alpha = static_cast<float>(clamp(m_alpha.get()));
	if (alpha == 0.0f) return;
	ColorTrans c(window, Color::alpha(alpha));
	m_texture.draw(window);
}
