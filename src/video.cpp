#include <video.h>

CVideo::CVideo(): mpeg() {}

bool CVideo::isPlaying() {
#ifdef USE_SMPEG
	if (mpeg) return (SMPEG_status(mpeg) == SMPEG_PLAYING);
#endif
#ifdef USE_FFMPEG_VIDEO
	if (mpeg) return mpeg->isPlaying();
#endif
	return false;
}

void CVideo::play() {
#ifdef USE_FFMPEG_VIDEO
	if (mpeg) mpeg->start();
#endif
#ifdef USE_SMPEG
	if (mpeg) SMPEG_play(mpeg);
#endif
}

void CVideo::unloadVideo() {
#ifdef USE_FFMPEG_VIDEO
	if (mpeg) delete mpeg;
#endif
#ifdef USE_SMPEG
	if (mpeg) SMPEG_delete(mpeg);
#endif
	mpeg = NULL;
}

void CVideo::update(double time) {
#ifdef USE_FFMPEG_VIDEO
	while (mpeg->videoQueue.size() > 0) {
		VideoFrame& fr = mpeg->videoQueue.front();
		std::cout << fr.timestamp << " <= " << time << std::endl;
		if (fr.timestamp >= time) {
			SDL_LockSurface(m_videoSurf);
			memcpy(m_videoSurf->pixels, &fr.data[0], fr.data.size());
			SDL_UnlockSurface(m_videoSurf);
			break;
		} else {
			++mpeg->videoQueue;
		}
	}
#endif
#ifdef USE_SMPEG
	return;
#endif
}

bool CVideo::loadVideo(std::string const& _videoFile, SDL_Surface* _videoSurf, int _width , int _height) {
	unloadVideo();
#ifdef USE_FFMPEG_VIDEO
	mpeg = new CFfmpeg(true, false, _videoFile, _width, _height);
	m_videoSurf = _videoSurf;
	return true;
#endif
#ifdef USE_SMPEG
	mpeg = SMPEG_new(_videoFile.c_str(), &info, 0);
	if(SMPEG_error(mpeg)) {
		fprintf(stderr, "SMPEG error: %s\n", SMPEG_error(mpeg));
		SMPEG_delete(mpeg);
		mpeg = NULL;
		return false;
	} else {
		SMPEG_setdisplay(mpeg, _videoSurf, NULL, NULL);
		SMPEG_enablevideo(mpeg, 1);
		SMPEG_enableaudio(mpeg, 0);
		SMPEG_setvolume(mpeg, 0);
		SMPEG_scaleXY(mpeg, _width , _height);
		return true;
	}
#else
	std::cerr << "Video file available, but USNG was compiled without video support" << std::endl;
	return false;
#endif
}

void CVideo::seek(double seek_pos) {
#ifdef USE_SMPEG
	SMPEG_rewind( mpeg );
	SMPEG_skip( mpeg, seek_pos );
#endif
#ifdef USE_FFMPEG_VIDEO
	seek_pos = seek_pos;
#endif
}
