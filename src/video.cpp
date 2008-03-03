#include <video.h>

void CVideo::unloadVideo() {
#ifdef USE_FFMPEG_VIDEO
	mpeg.reset();
#endif
}

void CVideo::render(double time) {
#ifdef USE_FFMPEG_VIDEO
	if (!mpeg) return;
	VideoFrame& fr = m_videoFrame;
	if (!fr.data.empty()) {
		if (time < fr.timestamp) return;
//		SDL_LockSurface(m_videoSurf);
//		memcpy(m_videoSurf->pixels, &fr.data[0], fr.data.size());
//		SDL_UnlockSurface(m_videoSurf);
	}
	while (mpeg->videoQueue.tryPop(fr) && fr.timestamp < time);
#else
	void(time);
#endif
}

bool CVideo::loadVideo(std::string const& _videoFile) {
#ifdef USE_FFMPEG_VIDEO
	mpeg.reset(new CFfmpeg(true, false, _videoFile));
	return true;
#else
	void(_videoFile); void(_videoSurf);
	return false;
#endif
}

