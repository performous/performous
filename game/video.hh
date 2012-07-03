#pragma once

#include "animvalue.hh"
#include "surface.hh"
#include "ffmpeg.hh"
#include <string>
   
/// class for playing videos  
class Video {
  public:
	/// opens given video file
	Video(std::string const& videoFile, double videoGap = 0.0);
	void prepare(double time);  ///< Load the current video frame into a texture
	void render(double time);  ///< Render the prepared video frame
	/// returns Dimensions of video clip
	Dimensions& dimensions() { return m_surface.dimensions; }
	/// returns Dimensions of video clip
	Dimensions const& dimensions() const { return m_surface.dimensions; }

  private:
	FFmpeg m_mpeg;
	double m_videoGap;
	VideoFrame m_videoFrame;
	Surface m_surface;
	double m_surfaceTime;
	double m_lastTime;
	AnimValue m_alpha;
};

