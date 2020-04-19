#pragma once

#include "animvalue.hh"
#include "texture.hh"
#include "ffmpeg.hh"
#include <future>
#include <string>
   
/// class for playing videos  
class Video {
  public:
	/// opens given video file
	Video(fs::path const& videoFile, double videoGap = 0.0);
        ~Video();
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
        bool m_quit{false};
        std::future<void> m_grabber;

        /// Video queue
        class Fifo {
            public:
                /// trys to pop a video frame from queue
                bool tryPop(Bitmap& f);
                /// Add frame to queue
                void push(Bitmap&& f);
                /// Clear and unlock the queue
                void reset();
                /// return timestamp of next frame to read
                double headPosition() const { return m_queue.front().timestamp; }
                /// return timestamp of next frame to read
                double backPosition() const { return m_queue.back().timestamp; }
                /// return current read position
                double position() { return m_readPosition; }

            private:
                std::deque<Bitmap> m_queue;
                mutable std::mutex m_mutex;
                std::condition_variable m_cond;
                static const unsigned m_max = 20;
                double m_readPosition = 0;
        } videoQueue;
};

