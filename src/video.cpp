#include <video.h>

CVideo::CVideo()
{
#ifdef USE_SMPEG
	mpeg=NULL;
#endif
}

CVideo::~CVideo()
{
#ifdef USE_SMPEG
	if(mpeg != NULL)
		unloadVideo();
#endif
}

bool CVideo::isPlaying()
{
#ifdef USE_SMPEG
	if(mpeg != NULL)
		return (SMPEG_status(mpeg) == SMPEG_PLAYING);
	else
		return false;
#endif
	return false;
}

void CVideo::play(void)
{
#ifdef USE_SMPEG
	if(mpeg != NULL)
		SMPEG_play(mpeg);
#endif
}

void CVideo::unloadVideo( void )
{
#ifdef USE_SMPEG
	if( mpeg != NULL ) {
		SMPEG_delete(mpeg);
		mpeg=NULL;
	}
#endif
}

void CVideo::loadVideo( char * _videoFile, SDL_Surface * _videoSurf, int _width , int _height )
{
#ifdef USE_SMPEG
	unloadVideo();
	mpeg = SMPEG_new(_videoFile, &info, 0);
	if( SMPEG_error( mpeg ) ) {
		fprintf( stderr, "SMPEG error: %s\n", SMPEG_error( mpeg ) );
		SMPEG_delete( mpeg );
		mpeg = NULL;
	} else {
		SMPEG_setdisplay(mpeg, _videoSurf, NULL, NULL);
		SMPEG_enablevideo(mpeg, 1);
		SMPEG_enableaudio(mpeg, 0);
		SMPEG_setvolume(mpeg, 0);
		SMPEG_scaleXY(mpeg, _width , _height );
	}
#else
	fprintf(stdout,"Video file was found, but Ultrastar-ng was compile without video support\n");
#endif
}
