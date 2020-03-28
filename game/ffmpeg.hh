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

/// single audio frame
struct AudioFrame {
	/// timestamp of audio frame
	double timestamp;
	/// audio data
	std::vector<std::int16_t> data;
	/// constructor
	template <typename InIt> AudioFrame(double ts, InIt begin, InIt end): timestamp(ts), data(begin, end) {}
	AudioFrame(): timestamp(getInf()) {} // EOF marker
};

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

class AudioBuffer {
	typedef std::recursive_mutex mutex;
  public:
	AudioBuffer(size_t size = 4320000): m_data(size) {}
	/// Reset from FFMPEG side (seeking to beginning or terminate stream)
	void reset();
	void quit();
	/// set samples per second
	void setSamplesPerSecond(unsigned sps) { m_sps = sps; }
	/// get samples per second
	unsigned getSamplesPerSecond() const { return m_sps; }
	fvec_t* makePreviewBuffer();
	void push(std::vector<std::int16_t> const& data, double timestamp);
	bool prepare(std::int64_t pos);
	bool operator()(float* begin, float* end, std::int64_t pos, float volume = 1.0f);
	bool eof(std::int64_t pos) const { return double(pos) / m_sps >= m_duration; }
	void setEof() { m_duration = double(m_pos) / m_sps; }
	double duration() const { return m_duration; }
	void setDuration(double seconds) { m_duration = seconds; }
	bool wantSeek() {
		// Are we already past the requested position? (need to seek backward or back to beginning)
		return m_posReq > 0 && m_posReq + m_sps * 2 /* seconds tolerance */ + m_data.size() < m_pos;
	}
  private:
	/// Handle waking up of input thread etc. whenever m_posReq is changed.
	void wakeups() {
		if (wantSeek()) reset();
		else if (condition()) m_cond.notify_one();
	}
	bool wantMore() { return m_pos < m_posReq + m_data.capacity() / 2; }
	/// Should the input stop waiting?
	bool condition() { return m_quit.load() || wantMore() || wantSeek(); }
	mutable mutex m_mutex;
	std::condition_variable_any m_cond;
	boost::circular_buffer<std::int16_t> m_data;
	size_t m_pos = 0;
	std::int64_t m_posReq = 0;
	unsigned m_sps = 0;
	double m_duration = getNaN();
	std::atomic<bool> m_quit{ false };
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
	FFmpeg(fs::path const& file, unsigned int rate = 0);
	~FFmpeg();
	void operator()(); ///< Thread runs here, don't call directly
	unsigned width = 0; ///< width of video
	unsigned height = 0; ///< height of video
	/// queue for video
	VideoFifo  videoQueue;
	/// queue for audio
	AudioBuffer  audioQueue;
	/** Seek to the chosen time. Will block until the seek is done, if wait is true. **/
	void seek(double time, bool wait = true);
	/// duration
	double duration() const;
	bool terminating() { return m_quit_future.wait_for(0s) == std::future_status::ready; }

	class eof_error: public std::exception {};
  private:
	void seek_internal();
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
	unsigned int m_rate = 0;
	std::promise<void> m_quit;
	std::future<void> m_quit_future = m_quit.get_future();
	std::atomic<double> m_seekTarget{ getNaN() };
	double m_position = 0.0;
	double m_duration = 0.0;
	// libav-specific variables
	int m_streamId = -1;
	int m_mediaType;  // enum AVMediaType
	std::unique_ptr<AVFormatContext, decltype(&avformat_close_input)> m_formatContext{nullptr, avformat_close_input};
	std::unique_ptr<AVCodecContext, decltype(&avcodec_free_context)> m_codecContext{nullptr, avcodec_free_context};
	std::unique_ptr<SwrContext, void(*)(SwrContext*)> m_resampleContext{nullptr, [] (auto p) { swr_close(p); swr_free(&p); }};
	std::unique_ptr<SwsContext, void(*)(SwsContext*)> m_swsContext{nullptr, sws_freeContext};
	// Make sure the thread starts only after initializing everything else
	std::unique_ptr<std::thread> m_thread;
	static std::mutex s_avcodec_mutex; // Used for avcodec_open/close (which use some static crap and are thus not thread-safe)
};

