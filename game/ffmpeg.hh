#pragma once

#include "chrono.hh"
#include "surface.hh"
#include "util.hh"
#include "libda/sample.hpp"
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
	bool tryPop(Bitmap& f) {
		std::unique_lock<std::mutex> l(m_mutex);
		if (m_queue.empty()) return false; // Nothing to deliver
		if (m_queue.front().buf.empty()) { m_eof = true; return false; }
		f = std::move(m_queue.front());
		m_queue.pop_front();
		m_cond.notify_all();
		m_timestamp = f.timestamp;
		return true;
	}
	/// Add frame to queue
	void push(Bitmap&& f) {
		std::unique_lock<std::mutex> l(m_mutex);
		m_cond.wait(l, [this]{ return m_queue.size() < m_max; });
		if (m_queue.empty()) m_timestamp = f.timestamp;
		m_queue.emplace_back(std::move(f));
	}
	/// Clear and unlock the queue
	void reset() {
		std::unique_lock<std::mutex> l(m_mutex);
		m_queue.clear();
		m_cond.notify_all();
		m_eof = false;
	}
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
	AudioBuffer(size_t size = 1000000): m_data(size) {}
	/// Reset from FFMPEG side (seeking to beginning or terminate stream)
	void reset() {
		{
			std::unique_lock<mutex> l(m_mutex);
			m_data.clear();
			m_pos = 0;
		}
		m_cond.notify_one();
	}
	void quit() {
		m_quit.store(true);
		m_cond.notify_one();
	}
	/// set samples per second
	void setSamplesPerSecond(unsigned sps) { m_sps = sps; }
	/// get samples per second
	unsigned getSamplesPerSecond() const { return m_sps; }
	void push(std::vector<std::int16_t> const& data, double timestamp) {
		std::unique_lock<mutex> l(m_mutex);
		m_cond.wait(l, [this]{ return condition(); });
		if (m_quit) return;
		if (timestamp < 0.0) {
			std::clog << "ffmpeg/warning: Negative audio timestamp " << timestamp << " seconds, frame ignored." << std::endl;
			return;
		}
		// Insert silence at the beginning if the stream starts later than 0.0
		if (m_pos == 0 && timestamp > 0.0) {
			m_pos = timestamp * m_sps;
			m_data.resize(m_pos, 0);
		}
		m_data.insert(m_data.end(), data.begin(), data.end());
		m_pos += data.size();
	}
	bool prepare(std::int64_t pos) {
		std::unique_lock<mutex> l(m_mutex, std::try_to_lock);
		if (!l.owns_lock()) return false;  // Didn't get lock, give up for now
		if (eof(pos)) return true;
		if (pos < 0) pos = 0;
		m_posReq = pos;
		wakeups();
		// Has enough been prebuffered already and is the requested position still within buffer
		return m_pos > m_posReq + m_data.capacity() / 16 && m_pos <= m_posReq + m_data.size();
	}
	bool operator()(float* begin, float* end, std::int64_t pos, float volume = 1.0f) {
		std::unique_lock<mutex> l(m_mutex);
		size_t idx = pos + m_data.size() - m_pos;
		size_t samples = end - begin;
		for (size_t s = 0; s < samples; ++s, ++idx) {
			if (idx < m_data.size()) begin[s] += volume * da::conv_from_s16(m_data[idx]);
		}
		m_posReq = std::max<std::int64_t>(0, pos + samples);
		wakeups();
		return !eof(pos);
	}
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
  struct SwrContext;
  struct SwsContext;
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
	void decodePacket();
	void processVideo(AVFrame* frame);
	void processAudio(AVFrame* frame);
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
	AVFormatContext* m_formatContext = nullptr;
	AVCodecContext* m_codecContext = nullptr;
	SwrContext* m_resampleContext = nullptr;
	SwsContext* m_swsContext = nullptr;
	// Make sure the thread starts only after initializing everything else
	std::unique_ptr<std::thread> m_thread;
	static std::mutex s_avcodec_mutex; // Used for avcodec_open/close (which use some static crap and are thus not thread-safe)
};

