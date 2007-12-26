#include "ffmpeg.hpp"
#include <stdexcept>
#include <iostream>

CFfmpeg::CFfmpeg(bool _decodeVideo, bool _decodeAudio) : m_type() {
	av_register_all();
	videoStream=-1;
	audioStream=-1;
	decodeVideo=_decodeVideo;
	decodeAudio=_decodeAudio;
	m_thread.reset(new boost::thread(boost::ref(*this)));
}

CFfmpeg::~CFfmpeg() {
	{
		boost::mutex::scoped_lock l(m_mutex);
		m_type = QUIT;
		m_cond.notify_all();
	}
	m_thread->join();
}

bool CFfmpeg::open( const char * _filename ) {
	if(av_open_input_file(&pFormatCtx, _filename, NULL, 0, NULL)!=0)
		return false;
	if(av_find_stream_info(pFormatCtx)<0)
		return false;

#ifdef DEBUG
	dump_format(pFormatCtx, 0, _filename, false);
	std::cerr << "Decode Video: " << decodeVideo << std::endl;
	std::cerr << "Decode Audio: " << decodeAudio << std::endl;
#endif

	videoStream=-1;
	audioStream=-1;
	// Take the first video stream
	for(int i=0; i<pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
			videoStream=i;
			break;
		}
	}
	// Take the first audio stream
	for(int i=0; i<pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO) {
			audioStream=i;
			break;
		}
	}

	try {	
		if( videoStream != -1 && decodeVideo ) {
			pVideoCodecCtx=pFormatCtx->streams[videoStream]->codec;
			pVideoCodec=avcodec_find_decoder(pVideoCodecCtx->codec_id);
			if( pVideoCodec == NULL )
				throw std::runtime_error("Cannot find video codec");
			if(pVideoCodec->capabilities & CODEC_CAP_TRUNCATED)
				pVideoCodecCtx->flags|=CODEC_FLAG_TRUNCATED;
			if(avcodec_open(pVideoCodecCtx, pVideoCodec)<0)
				throw std::runtime_error("Cannot open video stream");
		}
	} catch (std::runtime_error& e) {
		// TODO: clean memory
		std::cerr << "Failed to load video (" <<  e.what() << ")" <<std::endl;
		videoStream = -1;
	}
	try {	
		if( audioStream != -1 && decodeAudio ) {
			pAudioCodecCtx=pFormatCtx->streams[audioStream]->codec;
			pAudioCodec=avcodec_find_decoder(pAudioCodecCtx->codec_id);
			if( pAudioCodec == NULL )
				throw std::runtime_error("Cannot find audio codec");
			if(pAudioCodec->capabilities & CODEC_CAP_TRUNCATED)
				pAudioCodecCtx->flags|=CODEC_FLAG_TRUNCATED;
			if(avcodec_open(pAudioCodecCtx, pAudioCodec)<0)
				throw std::runtime_error("Cannot open audio stream");
		}
	} catch (std::runtime_error& e) {
		// TODO: clean memory
		std::cerr << "Failed to load audio (" <<  e.what() << ")" <<std::endl;
		audioStream = -1;
	}
}

void CFfmpeg::close( void ) {
	stop();
	if( videoStream != -1 && decodeVideo ) {
		avcodec_close(pVideoCodecCtx);
	}
	if( audioStream != -1 && decodeAudio ) {
		avcodec_close(pAudioCodecCtx);
	}
	av_close_input_file(pFormatCtx);
}

#include <boost/thread/xtime.hpp>
#include <boost/thread/thread.hpp>
#include <cmath>

namespace {
	boost::xtime& operator+=(boost::xtime& time, double seconds) {
		double nsec = 1e9 * (time.sec + seconds) + time.nsec;
		time.sec = boost::xtime::xtime_sec_t(nsec / 1e9);
		time.nsec = boost::xtime::xtime_nsec_t(std::fmod(nsec, 1e9));
		return time;
	}
	boost::xtime operator+(boost::xtime const& left, double seconds) {
		boost::xtime time = left;
		return time += seconds;
	}
	boost::xtime now() {
		boost::xtime time;
		boost::xtime_get(&time, boost::TIME_UTC);
		return time;
	}
}


void CFfmpeg::operator()() {
	for(;;) {
		Type type;
		{
			boost::mutex::scoped_lock l(m_mutex);
			m_ready = true;
			m_condready.notify_all();
			while (m_type == STOP) m_cond.wait(l);
			m_ready = false;
			type = m_type;
		}
		// TODO: Here we wait if queues are full (video or audio)
		// sleepd for 10ms
		switch( type ) {
			case STOP:
				break;
			case QUIT:
				return;
			case PLAY:
				if( videoQueue.frames.size() >= 10 || audioQueue.frames.size() >= 10 ) {
					boost::thread::sleep(now() + 0.01);
					break;
				} else {
					try {
						decodeNextFrame();
					} catch (std::exception& e) {
						std::cerr << "ffmpeg error: " << e.what() << std::endl;
					}
				}
				break;
			default:
				break;
		}
	}
}

void CFfmpeg::decodeNextFrame( void ) {
	AVPacket packet;
	int frameFinished=0;
	bool newVideoFrame = true;
	AVFrame * videoFrame;
	std::cout << "ENTERING DECODE FRAME ---" << std::endl;

	while(av_read_frame(pFormatCtx, &packet)>=0) {
		if(packet.stream_index==videoStream && decodeVideo) {
			if( newVideoFrame ) {
				std::cout << "Allocate new frame" << std::endl;
				videoFrame=avcodec_alloc_frame();
				newVideoFrame = false;
			}

			int decodeSize = avcodec_decode_video(pVideoCodecCtx, videoFrame, &frameFinished, packet.data, packet.size);

			std::cerr << "decodeSize x frameFinished: " << decodeSize << " x " << frameFinished << std::endl;

			if( decodeSize <= 0 ) {
				av_free_packet(&packet);
				throw std::runtime_error("cannot decode frame");
			}

			if(frameFinished) {
				float time = av_q2d(pFormatCtx->streams[videoStream]->time_base) * packet.pts;
				std::cout << "Video time: " << time << std::endl;

				videoQueue.frames.push(videoFrame);

				av_free_packet(&packet);
				return;
			}

			av_free_packet(&packet);
		} else if(packet.stream_index==audioStream && decodeAudio) {
			float time = av_q2d(pFormatCtx->streams[audioStream]->time_base) * packet.pts;
			std::cout << "Audio time: " << time << std::endl;
			av_free_packet(&packet);
		} else {
			// Not the first audio streamer nor the first video stream
			// Could be a second stream or a subtitle stream
			av_free_packet(&packet);
		}
	}
}

/*
 g++ -g -o test src/ffmpeg.cpp -Iinclude -DDEBUG -lavutil -lavformat -lavcodec -lboost_thread
*/

int main(int argc, char** argv) {

	if( argc != 2 ) {
		std::cerr << "Usage: " << argv[0] << " video_file" << std::endl;
		return EXIT_FAILURE;
	}
	// Choose to decode both audio and video
	CFfmpeg test(true,true);
	test.open(argv[1]);
	boost::thread::sleep(now() + 0.5 );
	test.start();
	boost::thread::sleep(now() + 1.0 );
	test.close();
	boost::thread::sleep(now() + 1.0 );
	return EXIT_SUCCESS;
}
