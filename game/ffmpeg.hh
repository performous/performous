#pragma once

#include "surface.hh"
#include "util.hh"
#include "libda/sample.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <memory>
#include <vector>
#include <iostream>

using boost::uint8_t;
using boost::int16_t;
using boost::int64_t;

/// single audio frame
struct AudioFrame {
	/// timestamp of audio frame
	double timestamp;
	/// audio data
	std::vector<int16_t> data;
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
		boost::mutex::scoped_lock l(m_mutex);
		if (m_queue.empty()) return false; // Nothing to deliver
		if (m_queue.begin()->buf.empty()) { m_eof = true; return false; }
		f.swap(*m_queue.begin());
		m_queue.pop_front();
		m_cond.notify_all();
		m_timestamp = f.timestamp;
		return true;
	}
	/// Add frame to queue
	void push(Bitmap* f) {
		boost::mutex::scoped_lock l(m_mutex);
		while (m_queue.size() > m_max) m_cond.wait(l);
		if (m_queue.empty()) m_timestamp = f->timestamp;
		m_queue.push_back(f);
	}
	/// Clear and unlock the queue
	void reset() {
		boost::mutex::scoped_lock l(m_mutex);
		m_queue.clear();
		m_cond.notify_all();
		m_eof = false;
	}
	/// Returns the current position (seconds)
	double position() const { return m_timestamp; }
	/// Tests if EOF has already been reached
	double eof() const { return m_eof; }

  private:
	boost::ptr_deque<Bitmap> m_queue;
	mutable boost::mutex m_mutex;
	boost::condition m_cond;
	double m_timestamp;
	bool m_eof;
	static const unsigned m_max = 20;
};

class AudioBuffer {
	typedef boost::recursive_mutex mutex;
  public:
	AudioBuffer(size_t size = 1000000): m_data(size), m_pos(), m_posReq(), m_sps(), m_duration(getNaN()), m_quit() {}
	/// Reset from FFMPEG side (seeking to beginning or terminate stream)
	void reset() {
		mutex::scoped_lock l(m_mutex);
		m_data.clear();
		m_pos = 0;
		l.unlock();
		m_cond.notify_one();
	}
	void quit() {
		mutex::scoped_lock l(m_mutex);
		m_quit = true;
		l.unlock();
		m_cond.notify_one();
	}
	/// set samples per second
	void setSamplesPerSecond(unsigned sps) { m_sps = sps; }
	/// get samples per second
	unsigned getSamplesPerSecond() const { return m_sps; }
	void push(std::vector<int16_t> const& data, double timestamp) {
		mutex::scoped_lock l(m_mutex);
		while (!condition()) m_cond.wait(l);
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
	bool prepare(int64_t pos) {
		mutex::scoped_try_lock l(m_mutex);
		if (!l.owns_lock()) return false;  // Didn't get lock, give up for now
		if (eof(pos)) return true;
		if (pos < 0) pos = 0;
		m_posReq = pos;
		wakeups();
		// Has enough been prebuffered already and is the requested position still within buffer
		return m_pos > m_posReq + m_data.capacity() / 16 && m_pos <= m_posReq + m_data.size();
	}
	bool operator()(float* begin, float* end, int64_t pos, float volume = 1.0f) {
		mutex::scoped_lock l(m_mutex);
		size_t idx = pos + m_data.size() - m_pos;
		size_t samples = end - begin;
		for (size_t s = 0; s < samples; ++s, ++idx) {
			if (idx < m_data.size()) begin[s] += volume * da::conv_from_s16(m_data[idx]);
		}
		m_posReq = std::max<int64_t>(0, pos + samples);
		wakeups();
		return !eof(pos);
	}
	bool eof(int64_t pos) const { return double(pos) / m_sps >= m_duration; }
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
	bool condition() { return m_quit || wantMore() || wantSeek(); }
	mutable mutex m_mutex;
	boost::condition m_cond;
	boost::circular_buffer<int16_t> m_data;
	size_t m_pos;
	int64_t m_posReq;
	unsigned m_sps;
	double m_duration;
	bool m_quit;
};

// ffmpeg forward declarations
extern "C" {
  struct AVCodecContext;
  struct AVFormatContext;
  struct AVFrame;
  struct AVAudioResampleContext;
  struct SwsContext;
}

/// ffmpeg class
class FFmpeg {
  public:
	/// Decode file; if no rate is specified, decode video, otherwise decode audio.
	FFmpeg(fs::path const& file, unsigned int rate = 0);
	~FFmpeg();
	void operator()(); ///< Thread runs here, don't call directly
	unsigned width; ///< width of video
	unsigned height; ///< height of video
	/// queue for video
	VideoFifo  videoQueue;
	/// queue for audio
	AudioBuffer  audioQueue;
	/** Seek to the chosen time. Will block until the seek is done, if wait is true. **/
	void seek(double time, bool wait = true);
	/// duration
	double duration() const;
	bool terminating() const { return m_quit; }

	class eof_error: public std::exception {};
  private:
	void seek_internal();
	void open();
	void decodePacket();
	void processVideo(AVFrame* frame);
	void processAudio(AVFrame* frame);
	fs::path m_filename;
	unsigned int m_rate;
	volatile bool m_quit;
	volatile double m_seekTarget;
	double m_position;
	double m_duration;
	// libav-specific variables
	int m_streamId;
	int m_mediaType;  // enum AVMediaType
	AVFormatContext* m_formatContext;
	AVCodecContext* m_codecContext;
	AVAudioResampleContext* m_resampleContext;
	SwsContext* m_swsContext;
	// Make sure the thread starts only after initializing everything else
	std::unique_ptr<boost::thread> m_thread;
	static boost::mutex s_avcodec_mutex; // Used for avcodec_open/close (which use some static crap and are thus not thread-safe)
};

