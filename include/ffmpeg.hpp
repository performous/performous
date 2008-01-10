#ifndef __FFMEG_HPP__
#define __FFMEG_HPP__

extern "C" {
	#define __STDC_CONSTANT_MACROS 
	#include <ffmpeg/avcodec.h>
	#include <ffmpeg/avformat.h>
}

#include <queue>

class AudioFrame {
	public:
		AudioFrame() {};
		~AudioFrame() {delete[] frame;};
		int16_t * frame;
		unsigned long bufferSize;
		float timestamp;
};

class VideoFrame {
	public:
		VideoFrame() {};
		~VideoFrame() {av_free(frame);};
		AVFrame * frame;
		float timestamp;
};

template <class T> class queuedIterator {
	public:
		queuedIterator() {};
		queuedIterator& operator++() { frames.pop(); return *this; };
		T& operator*() { return *frames.front();};
		T* operator->() { return frames.front();};
		const unsigned int size() {return frames.size();};
		const bool isFull() { return (size() >= 10);};
		void push(T f) {frames.push(f);};
		std::queue<T> frames;
	private:
};

#include <boost/scoped_ptr.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

class CFfmpeg {
	public:
		CFfmpeg(bool decodeVideo=false, bool decodeAudio=false);
		~CFfmpeg();
		bool open( const char * _filename );
		void close( void );
		void decodeNextFrame(void);
		void operator()(); // Thread runs here, don't call directly
		void stop() {
			boost::mutex::scoped_lock l(m_mutex);
			m_type = STOP;
			m_cond.notify_one();
		}
		void start() {
			boost::mutex::scoped_lock l(m_mutex);
			m_type = PLAY;
			m_cond.notify_one();
		}
		queuedIterator<VideoFrame*>  videoQueue;
		queuedIterator<AudioFrame*>  audioQueue;
	private:
		enum Type { STOP, PLAY, SEEK, PAUSE, QUIT } m_type;
		boost::mutex m_mutex;
		boost::condition m_cond;
		boost::condition m_condready;
		boost::scoped_ptr<boost::thread> m_thread;
		bool m_ready;

		AVFormatContext *pFormatCtx;
		ReSampleContext *pResampleCtx;

		AVCodecContext  *pVideoCodecCtx;
		AVCodecContext  *pAudioCodecCtx;
		AVCodec         *pVideoCodec;
		AVCodec         *pAudioCodec;


		int             videoStream;
		int             audioStream;
		bool            decodeVideo;
		bool            decodeAudio;
};

#endif
