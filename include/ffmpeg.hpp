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
	bool tryPop(VideoFrame& f) {
		boost::mutex::scoped_lock l(m_mutex);
		if (m_queue.size() < 3) return false; // Must keep a certain minimum size for B-frame reordering
		f.swap(*m_queue.begin());
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
	double position() {
		boost::mutex::scoped_lock l(m_mutex);
		double result;
		if (m_queue.empty()) {
			result = 0.;
		} else {
			VideoFrame& tmp = *m_queue.begin();
			result = tmp.timestamp;
		}
		return result;
	}
  private:
	boost::ptr_set<VideoFrame> m_queue;
	boost::mutex m_mutex;
	boost::condition m_cond;
};

class AudioFifo {
  public:
	std::size_t tryPop(std::vector<int16_t>& buffer,std::size_t _size = 0) {
		boost::mutex::scoped_lock l(m_mutex);
		if (m_queue.empty()) return 0;

		AudioFrame& tmp = m_queue.front();
		std::size_t size = tmp.data.size();
		if( _size == 0 || tmp.data.size() <= _size ) {
			buffer.insert( buffer.begin(), tmp.data.begin(), tmp.data.end() );
			m_queue.pop_front();
			m_cond.notify_one();
			return size;
		} else {
			buffer.insert( buffer.begin(), tmp.data.begin(), tmp.data.begin() + _size );
			tmp.data.erase(tmp.data.begin(), tmp.data.begin() + _size );
			return _size;
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
	double position() {
		boost::mutex::scoped_lock l(m_mutex);
		double result;
		if (m_queue.empty()) {
			result = 0.;
		} else {
			AudioFrame& tmp = m_queue.front();
			result = tmp.timestamp;
		}
		return result;
	}
  private:
	boost::ptr_deque<AudioFrame> m_queue;
	boost::mutex m_mutex;
	boost::condition m_cond;
};

#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

class CFfmpeg {
  public:
	CFfmpeg(bool decodeVideo, bool decodeAudio, std::string const& file);
	~CFfmpeg();
	void operator()(); // Thread runs here, don't call directly
	VideoFifo  videoQueue;
	AudioFifo  audioQueue;
	void seek(double time) { m_seekTarget = time; videoQueue.reset(); audioQueue.reset(); }
	double duration();
	double position() {return std::max(audioQueue.position(),videoQueue.position());};
  private:
	void seek_internal();
	void open(const char* _filename);
	void close();
	void decodeNextFrame();
	boost::scoped_ptr<boost::thread> m_thread;
	volatile bool m_quit;
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
