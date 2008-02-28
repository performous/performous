#ifndef __VIDEO_H_
#define __VIDEO_H_

#include "../config.h"
#include <string>

#ifdef USE_FFMPEG_VIDEO
#include "ffmpeg.hpp"
#endif
       
class CVideo {
  public:
	CVideo();
	~CVideo() { unloadVideo(); }
	bool loadVideo(std::string const& videoFile, SDL_Surface* videoSurf, int width, int height );
	void unloadVideo();
	bool isPlaying();
	void update( double time );
	void play();
	void seek(double seek_pos);
  private:
#ifdef USE_SMPEG
	SMPEG *mpeg;
	SMPEG_Info info;
#endif
#ifdef USE_FFMPEG_VIDEO
	CFfmpeg *mpeg;
	SDL_Surface *m_videoSurf;
#endif
};

#endif
