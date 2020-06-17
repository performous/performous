#pragma once

#include "chrono.hh"
#include "texture.hh"
#include "util.hh"
#include "libda/sample.hpp"
#include "aubio/aubio.h"
#include <boost/circular_buffer.hpp>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <future>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

/// Video queue
class VideoFifo {
  public:
	VideoFifo(): m_timestamp(), m_eof() {}
	/// trys to pop a video frame from queue
	bool tryPop(Bitmap& f);
	/// Add frame to queue
	void push(Bitmap&& f);
	/// Clear and unlock the queue
	void reset();
	/// Returns the current position (seconds)
	double position() const { return m_timestamp; }
	/// Tests if EOF has already been reached
	double eof() const { return m_eof; }

  private:
	std::deque<Bitmap> m_queue;
	mutable std::mutex m_mutex;
	std::condition_variable m_cond;
	double m_timestamp;
	bool m_eof;
	static const unsigned m_max = 20;
};

// ffmpeg forward declarations
extern "C" {
  struct AVCodecContext;
  struct AVFormatContext;
  struct AVFrame;
  void av_frame_free(AVFrame **);
  struct SwrContext;
  void swr_free(struct SwrContext **);
  void swr_close(struct SwrContext *);
  struct SwsContext;
  void sws_freeContext(struct SwsContext *);
}

/// ffmpeg class
class FFmpeg {
  public:
	/// Decode file; if no rate is specified, decode video, otherwise decode audio.
	FFmpeg(fs::path const& file, AudioBuffer *audioBuffer = nullptr);
	~FFmpeg();
	void operator()(); ///< Thread runs here, don't call directly
        void handleOneFrame();

	/// queue for video
	VideoFifo  videoQueue;
	
        /** Seek to the chosen time. Will block until the seek is done, if wait is true. **/
	void seek(double time);
	/// duration
	double duration() const { return m_duration; }
	bool terminating() { return m_quit_future.wait_for(0s) == std::future_status::ready; }

	class eof_error: public std::exception {};
	void seek_internal();
  private:
	void open();
        struct Packet;
	void decodePacket(Packet &);
	static void frameDeleter(AVFrame *f) { if (f) av_frame_free(&f); }
	using uFrame = std::unique_ptr<AVFrame, std::integral_constant<decltype(&frameDeleter), &frameDeleter>>;
	void processVideo(uFrame frame);
	void processAudio(uFrame frame);
	static void avformat_close_input(AVFormatContext *fctx);
	static void avcodec_free_context(AVCodecContext *avctx);

	fs::path m_filename;
        AudioBuffer *audioBuffer = nullptr;
	unsigned int m_rate = 0;
	std::promise<void> m_quit;
	std::future<void> m_quit_future = m_quit.get_future();
	std::atomic<double> m_seekTarget{ getNaN() };
	double m_position = 0.0;
	double m_duration = 0.0;
        int64_t m_position_in_48k_frames = -1;
	// libav-specific variables
	int m_streamId = -1;
	int m_mediaType;  // enum AVMediaType
	std::unique_ptr<AVFormatContext, decltype(&avformat_close_input)> m_formatContext{nullptr, avformat_close_input};
	std::unique_ptr<AVCodecContext, decltype(&avcodec_free_context)> m_codecContext{nullptr, avcodec_free_context};
	std::unique_ptr<SwrContext, void(*)(SwrContext*)> m_resampleContext{nullptr, [] (auto p) { swr_close(p); swr_free(&p); }};
	std::unique_ptr<SwsContext, void(*)(SwsContext*)> m_swsContext{nullptr, sws_freeContext};
	// Make sure the thread starts only after initializing everything else
	std::future<void> m_thread;
	static std::mutex s_avcodec_mutex; // Used for avcodec_open/close (which use some static crap and are thus not thread-safe)
};

class AudioBuffer {
	typedef std::recursive_mutex mutex;
  public:
        using uFvec = std::unique_ptr<fvec_t, std::integral_constant<decltype(&del_fvec), &del_fvec>>;

	AudioBuffer(fs::path const& file, unsigned int rate, size_t size = 4320256);
        ~AudioBuffer();
	
	uFvec makePreviewBuffer();
	void push(std::vector<std::int16_t> const& data, int64_t sample_position);
	bool prepare(std::int64_t pos);
	bool read(float* begin, size_t count, std::int64_t pos, float volume = 1.0f);
        bool terminating();
        auto duration() { return ffmpeg.duration(); }

  private:
	bool eof(std::int64_t pos) const { return m_eof_pos != -1 && pos >= m_eof_pos || (double(pos) / m_sps >= m_duration); }
	bool wantSeek();
	bool wantMore();
	/// Should the input stop waiting?
	bool condition();

	mutable mutex m_mutex;
	std::condition_variable_any m_cond;
	
	std::vector<std::int16_t> m_data;
	size_t m_write_pos = 0;
	std::int64_t m_read_pos = 0;
	std::int64_t m_eof_pos = -1; // -1 until we get the read end from ffmpeg

	const unsigned m_sps;

        FFmpeg ffmpeg;
        const double m_duration;

        bool m_seek_asked { false };

	bool m_quit{ false };
	std::future<void> reader_thread;
};

