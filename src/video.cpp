#include <video.h>

CVideo::CVideo() {
#ifdef USE_SMPEG
	mpeg=NULL;
#endif
}

CVideo::~CVideo() {
	unloadVideo();
}

bool CVideo::isPlaying() {
#ifdef USE_SMPEG
	if (mpeg) return (SMPEG_status(mpeg) == SMPEG_PLAYING);
#endif
	return false;
}

void CVideo::play() {
#ifdef USE_SMPEG
	if (mpeg) SMPEG_play(mpeg);
#endif
}

void CVideo::unloadVideo() {
#ifdef USE_SMPEG
	if (mpeg) {
		SMPEG_delete(mpeg);
		mpeg=NULL;
	}
#endif
}

bool CVideo::loadVideo(std::string const& _videoFile, SDL_Surface* _videoSurf, int _width , int _height) {
#ifdef USE_SMPEG
	unloadVideo();
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
	fprintf(stdout,"Video file was found, but Ultrastar-ng was compile without video support\n");
	return false;
#endif
}
