#pragma once

#include "animvalue.hh"
#include "texture.hh"
#include <deque>
#include <future>
#include <string>
   
/// class for playing videos  
class Video {
  public:
	/// opens given video file
	Video(fs::path const& videoFile, float videoGap = 0.0f);
	~Video();
	void prepare(float time);  ///< Load the current video frame into a texture
	void render(float time);  ///< Render the prepared video frame
	/// returns Dimensions of video clip
	Dimensions const& dimensions() const { return m_texture.dimensions; }

  private:
	const float m_videoGap;
	Texture m_texture;
	float m_textureTime;
	float m_readPosition = 0.0f;
	AnimValue m_alpha;
	bool m_quit{false};
	std::future<void> m_grabber;

	/// trys to pop a video frame from queue
	bool tryPop(Bitmap& f, float timestamp);
	/// Add frame to queue
	void push(Bitmap&& f);
	/// Clear and unlock the queue
	void reset();
	/// return timestamp of next frame to read
	float headPosition() const { return m_queue.front().timestamp; }
	/// return timestamp of next frame to read
	float backPosition() const { return m_queue.back().timestamp; }

	std::deque<Bitmap> m_queue;
	mutable std::mutex m_mutex;
	std::condition_variable m_cond;
	static const unsigned m_max = 20;
	bool m_seek_asked{false};
};

