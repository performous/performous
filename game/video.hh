#ifndef PERFORMOUS_VIDEO_HH
#define PERFORMOUS_VIDEO_HH

#include "surface.hh"
#include "ffmpeg.hh"
#include <string>
   
/// class for playing videos  
class Video {
  public:
	/// opens given video file
	Video(std::string const& videoFile);
	/// renders video
	void render(double time);
	/// returns Dimensions of video clip
	Dimensions& dimensions() { return m_surface.dimensions; }
	/// returns Dimensions of video clip
	Dimensions const& dimensions() const { return m_surface.dimensions; }

  private:
	FFmpeg m_mpeg;
	VideoFrame m_videoFrame;
	Surface m_surface;
	double m_surfaceTime;
	double m_lastTime;
	float m_alpha;
};

#endif

