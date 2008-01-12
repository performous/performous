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
		unsigned long nbFrames;
		float timestamp;
};

class VideoFrame {
	public:
		VideoFrame() {};
		~VideoFrame() {av_free(frame);};
		AVFrame * frame;
		float timestamp;
		unsigned long bufferSize;
};

class videoFifo {
	public:
		videoFifo() {};
		~videoFifo() {};
		VideoFrame& operator*() { return *queue.front();};
		VideoFrame* operator->() { return queue.front();};
		videoFifo& operator++() { queue.pop(); return *this; };
		const unsigned int size() {return queue.size();};
		const bool isFull() { return (size() >= 10);};
		void push(VideoFrame* f) {queue.push(f);};
	private:
		std::queue<VideoFrame*> queue;
};
#include <iostream>
class audioFifo {
	public:
		audioFifo() {currentFramePosition=0;};
		~audioFifo() {};
		const unsigned int size() {return queue.size();};
		const bool isFull() { return (size() >= 10);};
		void push(AudioFrame* f) {queue.push(f);};
		unsigned long copypop(void * buffer, unsigned long size=0, unsigned char channels=2, bool blocking=false) {
			if( this->size() == 0 ) {
				std::cerr << "Empty audio queue" << std::endl;
				return 0;
			}
			AudioFrame * tmp = queue.front();

			unsigned long currentBufferPosition = currentFramePosition * channels;

			if( size == 0 || (tmp->nbFrames - currentFramePosition) == size ) {
				// Fill the output buffer with the rest of the current frame
				if( size == 0 )
					memcpy(buffer, tmp->frame+currentBufferPosition, channels * (tmp->nbFrames - currentFramePosition) * sizeof(short));
				else
					memcpy(buffer, tmp->frame+currentBufferPosition, channels * size * sizeof(short));
				queue.pop();
				return size;
			} else if( tmp->nbFrames - currentFramePosition > size ) {
				memcpy(buffer, tmp->frame+currentBufferPosition, channels * size * sizeof(int16_t));
				currentFramePosition+=size;
				return size;
			} else if( tmp->nbFrames - currentFramePosition < size ) {
				unsigned long result = (tmp->nbFrames - currentFramePosition);
				memcpy(buffer, tmp->frame+currentBufferPosition, channels * (tmp->nbFrames - currentFramePosition) * sizeof(int16_t));
				currentFramePosition=0;
				queue.pop();
				return result;
			}
		};
	private:
		std::queue<AudioFrame*> queue;
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
		videoFifo  videoQueue;
		audioFifo  audioQueue;
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
