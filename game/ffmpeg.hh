#pragma once
#ifndef PERFORMOUS_FFMEG_HH
#define PERFORMOUS_FFMEG_HH

#include "util.hh"
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <vector>
#include <limits>
#include <stdint.h>

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

/// audio queue: first in first out
class AudioFifo {
  public:
	AudioFifo(): m_sps(), m_timestamp(), m_eof() {}
	/// set samples per second
	void setSamplesPerSecond(unsigned sps) { m_sps = sps; }
	/// get samples per second
	unsigned getSamplesPerSecond() { return m_sps; }
	/// pops audio frame from queue to buffer
	void tryPop(std::vector<int16_t>& buffer, std::size_t size = 0) {
		if (size == 0) size = std::numeric_limits<std::size_t>::max();
		boost::mutex::scoped_lock l(m_mutex);
		while (!m_queue.empty() && size > 0) {
			AudioFrame& f = m_queue.front();
			if (f.data.empty()) { m_eof = true; break; } // Empty frames are EOF markers
			if (f.data.size() <= size) {
				buffer.insert(buffer.end(), f.data.begin(), f.data.end());
				size -= f.data.size();
				m_timestamp = f.timestamp + double(size) / m_sps;
				m_queue.pop_front();
				m_cond.notify_one();
			} else {
				buffer.insert(buffer.end(), f.data.begin(), f.data.begin() + size);
				f.data.erase(f.data.begin(), f.data.begin() + size);
				if (m_sps > 0) f.timestamp += double(size) / m_sps; // Samples of the package used, increment timestamp
				m_timestamp = f.timestamp;
				size = 0;
			}
		}
	}
	/// pushes AudioFrame to queue
	void push(AudioFrame* f) {
		boost::mutex::scoped_lock l(m_mutex);
		while (m_queue.size() > m_max) m_cond.wait(l);
		if (m_queue.empty()) m_timestamp = f->timestamp;
		m_queue.push_back(f);
	}
	/// reset queue
	void reset() {
		boost::mutex::scoped_lock l(m_mutex);
		m_queue.clear();
		m_cond.notify_all();
		m_eof = false;
	}
	/// current position
	double position() const { return m_timestamp; }
	/// end of queue
	double eof() const { return m_eof; }
	/// fill percentage
	double percentage() const {
		boost::mutex::scoped_lock l(m_mutex);
		return double(m_queue.size()) / m_max;
	}

  private:
	boost::ptr_deque<AudioFrame> m_queue;
	mutable boost::mutex m_mutex;
	boost::condition m_cond;
	unsigned m_sps;
	double m_timestamp;
	bool m_eof;
	static const unsigned m_max = 100;
};

// ffmpeg forward declarations
extern "C" {
  struct AVCodec;
  struct AVCodecContext;
  struct AVFormatContext;
  struct ReSampleContext;
  struct SwsContext;
}

/// ffmpeg class
class FFmpeg {
  public:
	/// constructor
	FFmpeg(bool decodeVideo, bool decodeAudio, std::string const& file, unsigned int rate = 48000);
	~FFmpeg();
	/**
	* This function is called by the crash handler to indicate that FFMPEG has
	* crashed or has gotten stuck, and that the destructor should not wait for
	* it to finish before exiting.
	**/
	void crash() { m_thread.reset(); m_quit = true; }
	void operator()(); ///< Thread runs here, don't call directly
	unsigned width, ///< width of video
	         height; ///< height of video
	/// queue for video
	VideoFifo  videoQueue;
	/// queue for audio
	AudioFifo  audioQueue;
	/** Seek to the chosen time. Will block until the seek is done, if wait is true. **/
	void seek(double time, bool wait = true);
	/// duration
	double duration() const;
	/// return current position
	double position() { return std::max(videoQueue.position(), audioQueue.position()); }

  private:
	class eof_error: public std::exception {};
	void seek_internal();
	void open();
	void decodeNextFrame();
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

#endif
