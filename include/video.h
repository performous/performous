#ifndef __VIDEO_H_
#define __VIDEO_H_

#include "../config.h"
#include <string>

#ifdef USE_FFMPEG_VIDEO
#include "ffmpeg.hpp"
#endif
       
class CVideo {
  public:
	bool loadVideo(std::string const& videoFile);
	void unloadVideo();
	void render(double time, double screenWidth, double screenHeight);
  private:
#ifdef USE_FFMPEG_VIDEO
	GLuint m_texture;
	boost::scoped_ptr<CFfmpeg> mpeg;
	VideoFrame m_videoFrame;
	double m_time;
#endif
};

#endif

