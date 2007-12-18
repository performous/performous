#ifndef __FFMEG_HPP__
#define __FFMEG_HPP__

extern "C" {
	#define __STDC_CONSTANT_MACROS 
	#include <ffmpeg/avcodec.h>
	#include <ffmpeg/avformat.h>
}

#include <queue>

class queuedIterator {
	public:
		queuedIterator() {};
		queuedIterator& operator++() { av_free(frames.front()); frames.pop(); return *this; };
		AVFrame& operator*() { return *frames.front();};
		AVFrame* operator->() { return frames.front();};
		std::queue<AVFrame*> frames;
	private:
};

class CFfmpeg {
	public:
		CFfmpeg(bool decodeVideo=false, bool decodeAudio=false);
		~CFfmpeg();
		bool open( const char * _filename );
		void close( void );
		bool decodeNextFrame(void);
		void _run();
	private:
		AVFormatContext *pFormatCtx;

		AVCodecContext  *pVideoCodecCtx;
		AVCodecContext  *pAudioCodecCtx;
		AVCodec         *pVideoCodec;
		AVCodec         *pAudioCodec;

		queuedIterator  videoQueue;
		queuedIterator  audioQueue;

		int             videoStream;
		int             audioStream;
		bool            decodeVideo;
		bool            decodeAudio;
};

#endif
