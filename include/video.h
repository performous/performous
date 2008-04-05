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
	void render(double time);
  private:
#ifdef USE_FFMPEG_VIDEO
	CFfmpeg m_mpeg;
	VideoFrame m_videoFrame;
	boost::scoped_ptr<Surface> m_surface;
	double m_surfaceTime;
	double m_time;
	double m_lastTime;
	float m_alpha;
#endif
};

#endif

