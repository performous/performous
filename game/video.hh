#pragma once

#include "animvalue.hh"
#include "texture.hh"
#include "ffmpeg.hh"
#include <string>
   
/// class for playing videos  
class Video {
  public:
	/// opens given video file
	Video(fs::path const& videoFile, double videoGap = 0.0);
	void prepare(double time);  ///< Load the current video frame into a texture
	void render(double time);  ///< Render the prepared video frame
	/// returns Dimensions of video clip
	Dimensions& dimensions() { return m_texture.dimensions; }
	/// returns Dimensions of video clip
	Dimensions const& dimensions() const { return m_texture.dimensions; }

  private:
	FFmpeg m_mpeg;
	double m_videoGap;
	Bitmap m_videoFrame;
	Texture m_texture;
	double m_textureTime;
	double m_lastTime;
	AnimValue m_alpha;
};

