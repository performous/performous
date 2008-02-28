#ifndef __FFMEG_HPP__
#define __FFMEG_HPP__

extern "C" {
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#include <ffmpeg/swscale.h>
}

#include <boost/ptr_container/ptr_deque.hpp>
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
};

class VideoFifo {
  public:
	VideoFifo() {}
	~VideoFifo() {}
	VideoFrame& operator*() { return queue.front(); }
	VideoFrame* operator->() { return &queue.front(); }
	VideoFifo& operator++() { queue.pop_front(); return *this; }
	const unsigned int size() { return queue.size(); }
	const bool isFull() { return size() >= 10; }
	void push(VideoFrame* f) { queue.push_back(f); }
	VideoFrame& front() { return queue.front(); }
  private:
	boost::ptr_deque<VideoFrame> queue;
};

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
	CFfmpeg(bool decodeVideo=false, bool decodeAudio=false);
	~CFfmpeg();
	void open(const char * _filename, double _width=0, double _height=0);
	void close();
	void decodeNextFrame();
	void operator()(); // Thread runs here, don't call directly
	void stop() {
		boost::mutex::scoped_lock l(m_mutex);
		m_type = STOP;
		m_cond.notify_one();
	}
	bool isPlaying() {
		boost::mutex::scoped_lock l(m_mutex);
		return m_type == PLAY;
	}
	void start() {
		boost::mutex::scoped_lock l(m_mutex);
		m_type = PLAY;
		m_cond.notify_one();
	}
	VideoFifo  videoQueue;
	AudioFifo  audioQueue;
  private:
	enum Type { STOP, PLAY, SEEK, PAUSE, QUIT } m_type;
	boost::mutex m_mutex;
	boost::condition m_cond;
	boost::condition m_condready;
	boost::scoped_ptr<boost::thread> m_thread;
	bool m_ready;

	AVFormatContext* pFormatCtx;
	ReSampleContext* pResampleCtx;

	AVCodecContext* pVideoCodecCtx;
	AVCodecContext* pAudioCodecCtx;
	AVCodec* pVideoCodec;
	AVCodec* pAudioCodec;

	int videoStream;
	int audioStream;
	bool decodeVideo;
	bool decodeAudio;
	double width, height;
};

#endif
