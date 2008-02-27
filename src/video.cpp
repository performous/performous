#include <video.h>

CVideo::CVideo() {
#ifdef USE_SMPEG
	mpeg=NULL;
#endif
#ifdef USE_FFMPEG_VIDEO
	mpeg = new CFfmpeg(true,false);
#endif
}

CVideo::~CVideo() {
	unloadVideo();
#ifdef USE_FFMPEG_VIDEO
	delete mpeg;
#endif
}

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
	if (mpeg) mpeg->close();
#endif
#ifdef USE_SMPEG
	if (mpeg) {
		SMPEG_delete(mpeg);
		mpeg=NULL;
	}
#endif
}

void CVideo::update(double time) {
#ifdef USE_FFMPEG_VIDEO
	VideoFrame* fr=NULL;
	while( mpeg->videoQueue.size() > 0 ) {
		fr = mpeg->videoQueue.front();
		if( fr->timestamp > time )
			break;
		else
			++mpeg->videoQueue;
	}

	if( fr == NULL )
		return;
	else {
		SDL_LockSurface( m_videoSurf );
		memcpy( m_videoSurf->pixels, fr->buffer , fr->bufferSize);
		SDL_UnlockSurface( m_videoSurf );
	}
	
#endif
#ifdef USE_SMPEG
	return;
#endif
}

bool CVideo::loadVideo(std::string const& _videoFile, SDL_Surface* _videoSurf, int _width , int _height) {
#ifdef USE_FFMPEG_VIDEO
	mpeg->open(_videoFile.c_str(), _width, _height);
	m_videoSurf = _videoSurf;
	return true;
#endif
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

void CVideo::seek(double seek_pos) {
#ifdef USE_SMPEG
	SMPEG_rewind( mpeg );
	SMPEG_skip( mpeg, seek_pos );
#endif
#ifdef USE_FFMPEG_VIDEO
	seek_pos = seek_pos;
#endif
}
