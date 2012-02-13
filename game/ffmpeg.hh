#pragma once

#include "util.hh"
#include "libda/sample.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <vector>

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

/// single video frame
struct VideoFrame {
	/// timestamp of video frame
	double timestamp;
	int width,  ///< width of frame
	    height; ///< height of frame
	/// data array
	std::vector<uint8_t> data;
	/// constructor
	VideoFrame(double ts, int w, int h): timestamp(ts), width(w), height(h) {}
	VideoFrame(): timestamp(getInf()) {} // EOF marker
	/// swaps to VideoFrames
	void swap(VideoFrame& f) {
		std::swap(timestamp, f.timestamp);
		data.swap(f.data);
		std::swap(width, f.width);
		std::swap(height, f.height);
	}
};

static bool operator<(VideoFrame const& a, VideoFrame const& b) {
	return a.timestamp < b.timestamp;
}

/// video queue: first in first out
class VideoFifo {
  public:
	VideoFifo(): m_available(), m_timestamp(), m_eof() {}
	/// trys to pop a VideoFrame from queue
	bool tryPop(VideoFrame& f) {
		boost::mutex::scoped_lock l(m_mutex);
		if (!m_queue.empty() && m_queue.begin()->data.empty()) { m_eof = true; return false; }
		statsUpdate();
		if (m_available == 0) return false; // Nothing to deliver
		f.swap(*m_queue.begin());
		m_queue.erase(m_queue.begin());
		m_cond.notify_all();
		m_timestamp = f.timestamp;
		statsUpdate();
		return true;
	}
	/// pushes VideoFrames to queue
	void push(VideoFrame* f) {
		boost::mutex::scoped_lock l(m_mutex);
		while (m_queue.size() > m_max) m_cond.wait(l);
		if (m_queue.empty()) m_timestamp = f->timestamp;
		m_queue.insert(f);
		statsUpdate();
	}
	/// updates stats
	void statsUpdate() {
		m_available = std::max(0, int(m_queue.size()) - int(m_min));
		if (m_available == 0 && !m_queue.empty() && m_queue.rbegin()->data.empty()) m_available = m_queue.size() - 1;
	}
	/// resets video queue
	void reset() {
		boost::mutex::scoped_lock l(m_mutex);
		m_queue.clear();
		m_cond.notify_all();
		statsUpdate();
		m_eof = false;
	}
	/// returns current position
	double position() const { return m_timestamp; }
	/// returns m_available / m_max
	double percentage() const { return double(m_available) / m_max; }
	/// simple eof check
	double eof() const { return m_eof; }

  private:
	boost::ptr_set<VideoFrame> m_queue;
	mutable boost::mutex m_mutex;
	boost::condition m_cond;
	volatile unsigned m_available;
	double m_timestamp;
	bool m_eof;
	static const unsigned m_min = 16; // H.264 may have 16 consecutive B frames
	static const unsigned m_max = 50;
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
		if (m_pos == 0 && timestamp != 0.0) {
			std::clog << "ffmpeg/info: The first audio frame begins at " << timestamp << " seconds instead of zero." << std::endl;
			m_pos = timestamp * m_sps;
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
  struct AVCodec;
  struct AVCodecContext;
  struct AVFormatContext;
  struct ReSampleContext;
  struct SwsContext;
}

struct ReadFramePacket;

/// ffmpeg class
class FFmpeg {
  public:
	/// constructor
	FFmpeg(bool decodeVideo, bool decodeAudio, std::string const& file, unsigned int rate = 48000);
	~FFmpeg();
	void operator()(); ///< Thread runs here, don't call directly
	unsigned width, ///< width of video
	         height; ///< height of video
	/// queue for video
	VideoFifo  videoQueue;
	/// queue for audio
	AudioBuffer  audioQueue;
	/** Seek to the chosen time. Will block until the seek is done, if wait is true. **/
	void seek(double time, bool wait = true);
	/// duration
	double duration() const;
	/// return current position
	double position() { return videoQueue.position(); /* FIXME: remove */ }
	bool terminating() const { return m_quit; }

	class eof_error: public std::exception {};
  private:
	void seek_internal();
	void open();
	void decodePacket();
	int decodeVideoFrame(ReadFramePacket& packet);
	int decodeAudioFrame(ReadFramePacket& packet);
	std::string m_filename;
	unsigned int m_rate;
	volatile bool m_quit;
	volatile bool m_running;
	volatile bool m_eof;
	volatile double m_seekTarget;
	AVFormatContext* pFormatCtx;
	ReSampleContext* pResampleCtx;
	SwsContext* img_convert_ctx;

	AVCodecContext* pVideoCodecCtx;
	AVCodecContext* pAudioCodecCtx;
	AVCodec* pVideoCodec;
	AVCodec* pAudioCodec;

	int videoStream;
	int audioStream;
	bool decodeVideo;
	bool decodeAudio;
	double m_position;
	boost::scoped_ptr<boost::thread> m_thread;
	static boost::mutex s_avcodec_mutex; // Used for avcodec_open/close (which use some static crap and are thus not thread-safe)
};

