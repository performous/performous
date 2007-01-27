#ifndef __VIDEO_H_
#define __VIDEO_H_

#include "../config.h"
       
class CVideo {
	public:
	CVideo();
	~CVideo();
	void loadVideo(char * videoFile, SDL_Surface * videoSurf, int width , int height );
	void unloadVideo();
	bool isPlaying();
	void play(void);

        private:
#ifdef USE_SMPEG
	SMPEG *mpeg;
	SMPEG_Info info;
#endif
};

#endif
