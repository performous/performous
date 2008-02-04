#ifndef __VIDEO_H_
#define __VIDEO_H_

#include "../config.h"
#include <string>
       
class CVideo {
  public:
	CVideo();
	~CVideo();
	bool loadVideo(std::string const& videoFile, SDL_Surface* videoSurf, int width, int height );
	void unloadVideo();
	bool isPlaying();
	void play();
	void seek(double seek_pos);
  private:
#ifdef USE_SMPEG
	SMPEG *mpeg;
	SMPEG_Info info;
#endif
};

#endif
