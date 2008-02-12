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
			if( (pResampleCtx = audio_resample_init(2,pAudioCodecCtx->channels,48000,pAudioCodecCtx->sample_rate)) == NULL )
				throw std::runtime_error("Cannot create resampling context");
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
		audio_resample_close(pResampleCtx);
	}
	av_close_input_file(pFormatCtx);
}

#include <xtime.h>
#include <boost/thread/thread.hpp>
#include <cmath>

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
				if( videoQueue.isFull() || audioQueue.isFull()  ) {
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

	while(av_read_frame(pFormatCtx, &packet)>=0) {
		if(packet.stream_index==videoStream && decodeVideo) {
			if( newVideoFrame ) {
				videoFrame=avcodec_alloc_frame();
				newVideoFrame = false;
			}

			int decodeSize = avcodec_decode_video(pVideoCodecCtx, videoFrame, &frameFinished, packet.data, packet.size);

			if( decodeSize < 0 ) {
				av_free(videoFrame);
				av_free_packet(&packet);
				throw std::runtime_error("cannot decode video frame");
			}

			if(frameFinished) {
				float time;

				if( packet.pts != AV_NOPTS_VALUE ) {
					time = av_q2d(pFormatCtx->streams[videoStream]->time_base) * packet.pts;
				} else if( packet.dts != AV_NOPTS_VALUE ) {
					time = av_q2d(pFormatCtx->streams[videoStream]->time_base) * packet.dts;
				} else {
					time = 0.0;
				}

				VideoFrame * tmp = new VideoFrame();
				tmp->frame = videoFrame;
				tmp->bufferSize = 0;
				tmp->timestamp = time;
				videoQueue.push(tmp);

				av_free_packet(&packet);
				return;
			}

			av_free_packet(&packet);
		} else if(packet.stream_index==audioStream && decodeAudio) {
			int16_t * audioFrames = new int16_t[AVCODEC_MAX_AUDIO_FRAME_SIZE];
			int outsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*sizeof(int16_t);
			float time;

			int decodeSize = avcodec_decode_audio2( pAudioCodecCtx, audioFrames, &outsize, packet.data, packet.size);

			// No data decoded
			if( outsize == 0 ) {
				av_free_packet(&packet);
				delete[] audioFrames;
				return;
			}

			if( decodeSize < 0 ) {
				av_free_packet(&packet);
				delete[] audioFrames;
				throw std::runtime_error("cannot decode audio frame");
			}

			if( packet.pts != AV_NOPTS_VALUE ) {
				time = av_q2d(pFormatCtx->streams[audioStream]->time_base) * packet.pts;
			} else if( packet.dts != AV_NOPTS_VALUE ) {
				time = av_q2d(pFormatCtx->streams[audioStream]->time_base) * packet.dts;
			} else {
				time = 0.0;
			}

			int16_t * audioFramesResampled = new int16_t[AVCODEC_MAX_AUDIO_FRAME_SIZE];
			int nb_sample = audio_resample(pResampleCtx, audioFramesResampled, audioFrames, outsize/(pAudioCodecCtx->channels*2));
			delete[] audioFrames;

			AudioFrame * tmp = new AudioFrame();
			tmp->frame = audioFramesResampled;
			tmp->timestamp = time;
			tmp->nbFrames = nb_sample;
			audioQueue.push(tmp);

			av_free_packet(&packet);
			return;
		} else {
			// Not the first audio streamer nor the first video stream
			// Could be a second stream or a subtitle stream
			av_free_packet(&packet);
		}
	}
	return;
}

/*
 * For PA18:
 g++ -g -o test src/ffmpeg.cpp -Iinclude -DDEBUG -lavutil -lavformat -lavcodec -lboost_thread -lportaudio
 * For PA19:
 g++ -g -o test src/ffmpeg.cpp -Iinclude -DDEBUG -lavutil -lavformat -lavcodec -lboost_thread `pkg-config --cflags --libs portaudio-2.0`
*/

#include <portaudio.h>

// Send number of channels through userdata, as well as the deocing audio
static int c_callback(void*, void* output, unsigned long framesPerBuffer, PaTimestamp, void* userdata) {

	audioFifo* audioQueue = static_cast<audioFifo*>(userdata);
	unsigned long size = 0;
	unsigned char channels = 2;
	float timestamp;
	while(size < framesPerBuffer) {
		int16_t* buf = (int16_t*)output;
		buf+=channels*size;
		size+=audioQueue->copypop(buf, framesPerBuffer-size, channels,&timestamp);
		std::cout << "Now playing @"<< timestamp << std::endl;
	}
	return 0;
}

int main(int argc, char** argv) {

	if( argc != 2 ) {
		std::cerr << "Usage: " << argv[0] << " video_file" << std::endl;
		return EXIT_FAILURE;
	}

	PaError pa_err=Pa_Initialize();
	if( pa_err != paNoError) {
		std::cerr << "PortAudio error: " << Pa_GetErrorText(pa_err) << std::endl;
		return EXIT_FAILURE;
	}

	// Choose to decode both audio and video
	CFfmpeg test(false,true);
	test.open(argv[1]);
	boost::thread::sleep(now() + 0.5 );
	test.start();


	PaStream *stream;
	pa_err = Pa_OpenStream(&stream, paNoDevice, 0, paInt16, NULL, \
			Pa_GetDefaultOutputDeviceID(), \
			2, paInt16, NULL, 48000, 1254, 0, 0, \
			c_callback, &test.audioQueue);
	if( pa_err != paNoError ) {
		std::cerr << "PortAudio error: " << Pa_GetErrorText(pa_err) << std::endl;
		return EXIT_FAILURE;
	}
	pa_err=Pa_StartStream(stream);
	if( pa_err != paNoError ) {
		std::cerr << "PortAudio error: " << Pa_GetErrorText(pa_err) << std::endl;
		return EXIT_FAILURE;
	}


	boost::thread::sleep(now() + 100.0 );
	test.close();
	boost::thread::sleep(now() + 1.0 );

	pa_err=Pa_CloseStream(stream);
	if( pa_err != paNoError ) {
		std::cerr << "PortAudio error: " << Pa_GetErrorText(pa_err) << std::endl;
		return EXIT_FAILURE;
	}
	pa_err=Pa_Terminate();
	if( pa_err != paNoError ) {
		std::cerr << "PortAudio error: " << Pa_GetErrorText(pa_err) << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
