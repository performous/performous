#include <video.h>

CVideo::CVideo()
#ifdef USE_FFMPEG_VIDEO
  : mpeg()
#endif
{}

bool CVideo::isPlaying() {
#ifdef USE_FFMPEG_VIDEO
	if (mpeg) return mpeg->isPlaying();
#endif
	return false;
}

void CVideo::play() {
#ifdef USE_FFMPEG_VIDEO
	if (mpeg) mpeg->start();
#endif
}

void CVideo::unloadVideo() {
#ifdef USE_FFMPEG_VIDEO
	if (mpeg) delete mpeg;
	mpeg = NULL;
#endif
}

void CVideo::update(double time) {
#ifdef USE_FFMPEG_VIDEO
	while (mpeg->videoQueue.size() > 0) {
		VideoFrame& fr = mpeg->videoQueue.front();
		if (fr.timestamp >= time) {
			SDL_LockSurface(m_videoSurf);
			memcpy(m_videoSurf->pixels, &fr.data[0], fr.data.size());
			SDL_UnlockSurface(m_videoSurf);
			break;
		} else {
			++mpeg->videoQueue;
		}
	}
#else
	(void)time;
#endif
}

bool CVideo::loadVideo(std::string const& _videoFile, SDL_Surface* _videoSurf) {
	unloadVideo();
#ifdef USE_FFMPEG_VIDEO
	mpeg = new CFfmpeg(true, false, _videoFile);
	m_videoSurf = _videoSurf;
	return true;
#else
	(void)_videoFile; (void)_videoSurf;
	return false;
#endif
}

void CVideo::seek(double seek_pos) {
#ifdef USE_FFMPEG_VIDEO
	seek_pos = seek_pos;
#else
	(void)seek_pos;
#endif
}
