#ifndef __FFMEG_HPP__
#define __FFMEG_HPP__

#include <../config.h>

#ifdef USE_FFMPEG_VIDEO

#ifdef USE_NEW_FFMPEG_VIDEO
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#endif

#ifdef USE_OLD_FFMPEG_VIDEO
extern "C" {
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#include <ffmpeg/swscale.h>
}
#endif

#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_set.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <iostream>
#include <vector>

struct AudioFrame {
	double timestamp;
	std::vector<int16_t> data;
};

struct VideoFrame {
	double timestamp;
	std::vector<uint8_t> data; 
	int width, height;
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
		if (m_queue.size() < 3) return false; // Must keep a certain minimum size for B-frame reordering
		f.swap(*m_queue.begin());
		m_timestamp = m_queue.begin()->timestamp;
		m_queue.erase(m_queue.begin());
		m_cond.notify_one();
		return true;
	}
	void push(VideoFrame* f) {
		boost::mutex::scoped_lock l(m_mutex);
		while (m_queue.size() > 10) m_cond.wait(l);
		m_queue.insert(f);
	}
	void reset() {
		boost::mutex::scoped_lock l(m_mutex);
		m_queue.clear();
		m_cond.notify_all();
	}
	double position() { return m_timestamp; }
  private:
	boost::ptr_set<VideoFrame> m_queue;
	boost::mutex m_mutex;
	boost::condition m_cond;
	double m_timestamp;
};

class AudioFifo {
  public:
	AudioFifo(): m_timestamp(), m_eof() {}
	void tryPop(std::vector<int16_t>& buffer, std::size_t size = 0) {
		if (size == 0) size = std::numeric_limits<std::size_t>::max();
		boost::mutex::scoped_lock l(m_mutex);
		while (!m_queue.empty() && size > 0) {
			std::vector<int16_t>& data = m_queue.front().data;
			if (data.empty()) { m_eof = true; break; } // Empty frames are EOF markers
			if (data.size() <= size) {
				buffer.insert(buffer.end(), data.begin(), data.end());
				size -= data.size();
				m_timestamp = m_queue.front().timestamp; // TODO: + data.size() / samplerate
				m_queue.pop_front();
				m_cond.notify_one();
			} else {
				buffer.insert(buffer.end(), data.begin(), data.begin() + size);
				data.erase(data.begin(), data.begin() + size);
				size = 0;
			}
		}
	}
	void push(AudioFrame* f) {
		boost::mutex::scoped_lock l(m_mutex);
		while (m_queue.size() > 10) m_cond.wait(l);
		m_queue.push_back(f);
	}
	void reset() {
		boost::mutex::scoped_lock l(m_mutex);
		m_queue.clear();
		m_cond.notify_all();
	}
	double position() { return m_timestamp; }
	double eof() { return m_eof; }
  private:
	boost::ptr_deque<AudioFrame> m_queue;
	boost::mutex m_mutex;
	boost::condition m_cond;
	double m_timestamp;
	bool m_eof;
};

#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

class CFfmpeg {
  public:
	CFfmpeg(bool decodeVideo, bool decodeAudio, std::string const& file);
	~CFfmpeg();
	void crash();
	void operator()(); // Thread runs here, don't call directly
	VideoFifo  videoQueue;
	AudioFifo  audioQueue;
	void seek(double time) { m_seekTarget = time; videoQueue.reset(); audioQueue.reset(); }
	double duration();
	double position() {return std::max(audioQueue.position(),videoQueue.position());};
  private:
	class eof_error: public std::exception {};
	void seek_internal();
	void open();
	void close();
	void decodeNextFrame();
	boost::scoped_ptr<boost::thread> m_thread;
	std::string m_filename;
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
};

#endif
#endif
