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
#ifdef USE_FFMPEG_VIDEO
	Video(std::string const& videoFile);
	void render(double time, double = 0.0, double = 0.0, double = 1.0, double = 0.0);
#else
	Video(std::string const&) {}
	void render(double, double, double, double, double) {}
#endif
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

