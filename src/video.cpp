#include <video.h>

#ifdef USE_FFMPEG_VIDEO
Video::Video(std::string const& _videoFile): m_mpeg(true, false, _videoFile), m_lastTime()
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
		surface.reset(new Surface(fr.width, fr.height, Surface::RGB, &fr.data[0]));
		fr.data.clear();
	}
	if( surface ) surface->draw(0.5,0.5,w,h);
	if (time < m_lastTime) m_mpeg.seek(time);
	m_lastTime = time;
	// Preload the next future frame
	if (fr.data.empty()) while (m_mpeg.videoQueue.tryPop(fr) && fr.timestamp < time);
#else
	(void)time; (void)w; (void)h;
#endif
}

