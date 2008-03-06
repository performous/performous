#ifndef __FFMEG_HPP__
#define __FFMEG_HPP__

#include <../config.h>

#ifdef USE_FFMPEG_VIDEO

extern "C" {
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#include <ffmpeg/swscale.h>
}

#include <boost/ptr_container/ptr_deque.hpp>
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

class VideoFifo {
  public:
	bool tryPop(VideoFrame& f) {
		boost::mutex::scoped_lock l(m_mutex);
		if (m_queue.empty()) return false;
		f.swap(m_queue.front());
		m_queue.pop_front();
		m_cond.notify_one();
		return true;
	}
	void push(VideoFrame* f) {
		boost::mutex::scoped_lock l(m_mutex);
		while (m_queue.size() > 10) m_cond.wait(l);
		m_queue.push_back(f);
	}
	void reset() {
		boost::mutex::scoped_lock l(m_mutex);
		m_queue.clear();
		m_cond.notify_all();
	}
  private:
	boost::ptr_deque<VideoFrame> m_queue;
	boost::mutex m_mutex;
	boost::condition m_cond;
};

// TODO: fix this to work similarly to VideoFifo
class AudioFifo {
  public:
	AudioFifo() { currentFramePosition=0; }
	const unsigned int size() { return queue.size(); }
	const bool isFull() { return (size() >= 10); }
	void push(AudioFrame* f) { queue.push_back(f); }
	unsigned long copypop(void * buffer, unsigned long _size, unsigned char channels, float * timestamp=NULL, bool blocking=false) {
		blocking=blocking;
		if (this->size() == 0) {
			if (timestamp != NULL) *timestamp = -1;
			memset(buffer,0x00, channels * _size * sizeof(short));
			std::cerr << "Empty audio queue" << std::endl;
			return _size;
		}
		AudioFrame& tmp = queue.front();

		if (timestamp) *timestamp = tmp.timestamp;

		unsigned long currentBufferPosition = currentFramePosition * channels;

		std::size_t frames = tmp.data.size();
		if (_size == 0 || (frames - currentFramePosition) == _size) {
			// Fill the output buffer with the rest of the current frame
			size_t s = (_size == 0 ? frames - currentFramePosition : _size);
			memcpy(buffer, &tmp.data[currentBufferPosition], channels * s * sizeof(short));
			queue.pop_front();
			return _size;
		} else if (frames - currentFramePosition > _size) {
			memcpy(buffer, &tmp.data[currentBufferPosition], channels * _size * sizeof(int16_t));
			currentFramePosition+=_size;
			return _size;
		} else if (frames - currentFramePosition < _size) {
			unsigned long result = (frames - currentFramePosition);
			memcpy(buffer, &tmp.data[currentBufferPosition], channels * (frames - currentFramePosition) * sizeof(int16_t));
			currentFramePosition=0;
			queue.pop_front();
			return result;
		}
	}
  private:
	boost::ptr_deque<AudioFrame> queue;
	unsigned long currentFramePosition;
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
	void seek(double time) {} // TODO
  private:
	void open(const char* _filename);
	void close();
	void decodeNextFrame();
	boost::scoped_ptr<boost::thread> m_thread;
	volatile bool m_quit;

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
};

#endif
#endif
