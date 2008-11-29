#ifndef __FFMEG_HPP__
#define __FFMEG_HPP__

#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <vector>
#include <limits>

struct AudioFrame {
	double timestamp;
	std::vector<int16_t> data;
	template <typename InIt> AudioFrame(double ts, InIt begin, InIt end): timestamp(ts), data(begin, end) {}
	AudioFrame(): timestamp(std::numeric_limits<double>::infinity()) {} // EOF marker
};

struct VideoFrame {
	double timestamp;
	int width, height;
	std::vector<uint8_t> data; 
	VideoFrame(double ts, int w, int h): timestamp(ts), width(w), height(h) {}
	VideoFrame(): timestamp(std::numeric_limits<double>::infinity()) {} // EOF marker (not used ATM)
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

class VideoFifo {
  public:
	VideoFifo(): m_timestamp() {}
	bool tryPop(VideoFrame& f) {
		boost::mutex::scoped_lock l(m_mutex);
		if (m_queue.size() < m_min) return false; // Must keep a certain minimum size for B-frame reordering
		f.swap(*m_queue.begin());
		m_queue.erase(m_queue.begin());
		m_cond.notify_all();
		m_timestamp = f.timestamp;
		statsUpdate();
		return true;
	}
	void push(VideoFrame* f) {
		boost::mutex::scoped_lock l(m_mutex);
		while (m_queue.size() > m_max) m_cond.wait(l);
		if (m_queue.empty()) m_timestamp = f->timestamp;
		m_queue.insert(f);
		statsUpdate();
	}
	void statsUpdate() { m_available = std::max(0, int(m_queue.size()) - int(m_min)); }
	void reset() {
		boost::mutex::scoped_lock l(m_mutex);
		m_queue.clear();
		m_cond.notify_all();
		statsUpdate();
	}
	double position() { return m_timestamp; }
	double percentage() const { return double(m_available) / m_max; }
  private:
	boost::ptr_set<VideoFrame> m_queue;
	boost::mutex m_mutex;
	boost::condition m_cond;
	volatile unsigned m_available;
	static const unsigned m_min = 3;
	static const unsigned m_max = 50;
	double m_timestamp;
};

class AudioFifo {
  public:
	AudioFifo(): m_sps(), m_timestamp(), m_eof() {}
	void setSamplesPerSecond(unsigned sps) { m_sps = sps; }
	unsigned getSamplesPerSecond() { return m_sps; }
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
	void push(AudioFrame* f) {
		boost::mutex::scoped_lock l(m_mutex);
		while (m_queue.size() > m_max) m_cond.wait(l);
		if (m_queue.empty()) m_timestamp = f->timestamp;
		m_queue.push_back(f);
	}
	void reset() {
		boost::mutex::scoped_lock l(m_mutex);
		m_queue.clear();
		m_cond.notify_all();
	}
	double position() { return m_timestamp; }
	double eof() { return m_eof; }
	double percentage() {
		boost::mutex::scoped_lock l(m_mutex);
		return double(m_queue.size()) / m_max;
	}
  private:
	boost::ptr_deque<AudioFrame> m_queue;
	boost::mutex m_mutex;
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

class CFfmpeg {
  public:
	CFfmpeg(bool decodeVideo, bool decodeAudio, std::string const& file, unsigned int rate = 48000);
	~CFfmpeg();
	/**
	* This function is called by the crash handler to indicate that FFMPEG has
	* crashed or has gotten stuck, and that the destructor should not wait for
	* it to finish before exiting.
	**/
	void crash() { m_thread.reset(); }
	void operator()(); // Thread runs here, don't call directly
	unsigned width, height;
	VideoFifo  videoQueue;
	AudioFifo  audioQueue;
	/** Seek to the chosen time. Will block until the seek is done, if wait is true. **/
	void seek(double time, bool wait = true);
	double duration();
	double position() { return std::max(audioQueue.position(),videoQueue.position()); };
  private:
	class eof_error: public std::exception {};
	void seek_internal();
	void open();
	void close();
	void decodeNextFrame();
	boost::scoped_ptr<boost::thread> m_thread;
	std::string m_filename;
	unsigned int m_rate;
	volatile bool m_quit;
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
	static boost::mutex s_avcodec_mutex; // Used for avcodec_open/close (which use some static crap and are thus not thread-safe)
};

#endif
