#ifndef __VIDEO_H_
#define __VIDEO_H_

#include "../config.h"
#include <string>

#include "surface.h"

#ifdef USE_FFMPEG_VIDEO
#include "ffmpeg.hpp"
#endif
       
class Video {
  public:
	Video(std::string const& videoFile);
	~Video();
	void render(double time, double screenWidth, double screenHeight);
  private:
#ifdef USE_FFMPEG_VIDEO
	CFfmpeg m_mpeg;
	boost::scoped_ptr<Surface> surface;
	VideoFrame m_videoFrame;
	double m_time;
	double m_lastTime;
#endif
};

#endif

