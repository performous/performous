#ifndef __FFMEG_HPP__
#define __FFMEG_HPP__

#include <ffmpeg/avformat.h>
#include <ffmpeg/swscale.h>
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
		~VideoFrame() {delete[] buffer;};
		float timestamp;
		uint8_t *buffer; 
		unsigned long bufferSize;
		int width;
		int height;
};

class videoFifo {
	public:
		videoFifo() {};
		~videoFifo() {};
		VideoFrame& operator*() { return *queue.front();};
		VideoFrame* operator->() { return queue.front();};
		videoFifo& operator++() { delete queue.front();queue.pop(); return *this; };
		const unsigned int size() {return queue.size();};
		const bool isFull() { return (size() >= 10);};
		void push(VideoFrame* f) {queue.push(f);};
		VideoFrame* front() { return queue.front();};
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
		unsigned long copypop(void * buffer, unsigned long _size, unsigned char channels, float * timestamp=NULL, bool blocking=false) {
			blocking=blocking;
			if( this->size() == 0 ) {
				if( timestamp != NULL )
					*timestamp = -1;
				memset(buffer,0x00, channels * _size * sizeof(short));
				std::cerr << "Empty audio queue" << std::endl;
				return _size;
			}
			AudioFrame * tmp = queue.front();

			if( timestamp != NULL )
				*timestamp = tmp->timestamp;

			unsigned long currentBufferPosition = currentFramePosition * channels;

			if( _size == 0 || (tmp->nbFrames - currentFramePosition) == _size ) {
				// Fill the output buffer with the rest of the current frame
				if( _size == 0 )
					memcpy(buffer, tmp->frame+currentBufferPosition, channels * (tmp->nbFrames - currentFramePosition) * sizeof(short));
				else
					memcpy(buffer, tmp->frame+currentBufferPosition, channels * _size * sizeof(short));
				queue.pop();
				return _size;
			} else if( tmp->nbFrames - currentFramePosition > _size ) {
				memcpy(buffer, tmp->frame+currentBufferPosition, channels * _size * sizeof(int16_t));
				currentFramePosition+=_size;
				return _size;
			} else if( tmp->nbFrames - currentFramePosition < _size ) {
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
		void open( const char * _filename, double _width=0, double _height=0 );
		void close( void );
		void decodeNextFrame(void);
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
		double		width;
		double		height;
};

#endif
