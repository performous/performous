#ifndef PERFORMOUS_VIDEO_HH
#define PERFORMOUS_VIDEO_HH

#include "surface.hh"
#include "ffmpeg.hh"
#include <string>
       
class Video {
  public:
	Video(std::string const& videoFile);
	void render(double time);
  private:
	CFfmpeg m_mpeg;
	VideoFrame m_videoFrame;
	boost::scoped_ptr<Surface> m_surface;
	double m_surfaceTime;
	double m_time;
	double m_lastTime;
	float m_alpha;
};

#endif

