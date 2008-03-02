#include "ffmpeg.hpp"
#include <stdexcept>
#include <iostream>

CFfmpeg::CFfmpeg(bool _decodeVideo, bool _decodeAudio, std::string const& _filename, double _width, double _height) : m_type() {
	av_register_all();
	videoStream=-1;
	audioStream=-1;
	decodeVideo=_decodeVideo;
	decodeAudio=_decodeAudio;
	open(_filename.c_str(), _width, _height);
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

void CFfmpeg::open( const char * _filename, double _width, double _height ) {
	if(av_open_input_file(&pFormatCtx, _filename, NULL, 0, NULL))
	  throw std::runtime_error("Cannot open input file");
	if(av_find_stream_info(pFormatCtx) < 0)
	  throw std::runtime_error("Cannot find stream information");

	width = _width;
	height = _height;
#ifdef DEBUG
	dump_format(pFormatCtx, 0, _filename, false);
	std::cerr << "Decode Video: " << decodeVideo << std::endl;
	std::cerr << "Decode Audio: " << decodeAudio << std::endl;
#endif

	videoStream=-1;
	audioStream=-1;
	// Take the first video stream
	for(unsigned int i=0; i<pFormatCtx->nb_streams; i++) {
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
			videoStream=i;
			break;
		}
	}
	// Take the first audio stream
	for(unsigned int i=0; i<pFormatCtx->nb_streams; i++) {
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
			if( width == 0 ) width = pVideoCodecCtx->width;
			if( height == 0 ) height = pVideoCodecCtx->height;
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
	// Setup software scaling context
	int w = pVideoCodecCtx->width;
	int h = pVideoCodecCtx->height;
	img_convert_ctx = sws_getContext(
	  w, h, pVideoCodecCtx->pix_fmt,
	  width, height, PIX_FMT_RGB32,
	  SWS_BICUBIC, NULL, NULL, NULL);
}

void CFfmpeg::close() {
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
		// FIXME: Use boost::condition for waiting instead of sleep
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

void CFfmpeg::decodeNextFrame() {
	AVPacket packet;
	int frameFinished=0;
	bool newVideoFrame = true;
	AVFrame* videoFrame = NULL;
	int videoBytesRemaining=0;
	uint8_t *videoRawData=NULL;

	while(av_read_frame(pFormatCtx, &packet)>=0) {
		// If we have a video frame we want to decode
		// we decode all the video frame from this paket
		// and only return when we decode a complete frame AND a complete packet
		// we do not return if
		//  - we have more frame in the packet
		//  - we have decoded an unfinished frame at the end of the packet
		if(packet.stream_index==videoStream) {
			if( !decodeVideo ) {
				av_free_packet(&packet);
				return;
			}
			if( newVideoFrame ) {
				videoFrame=avcodec_alloc_frame();
				newVideoFrame = false;
			}

			videoBytesRemaining=packet.size;
			videoRawData=packet.data;
			int decodeSize;

			while(videoBytesRemaining>0) {
				decodeSize = avcodec_decode_video(pVideoCodecCtx, videoFrame, &frameFinished, videoRawData, videoBytesRemaining);
				videoRawData+=decodeSize;
				videoBytesRemaining-=decodeSize;

				if(frameFinished) {
					float time;
				
					if( (uint64_t)packet.pts != AV_NOPTS_VALUE ) {
						time = av_q2d(pFormatCtx->streams[videoStream]->time_base) * packet.pts;
					} else if( (uint64_t)packet.dts != AV_NOPTS_VALUE ) {
						time = av_q2d(pFormatCtx->streams[videoStream]->time_base) * packet.dts;
					} else {
						time = 0.0;
					}
				
					std::vector<uint8_t> buffer(width * height * 4);
					{
						uint8_t* data[] = { &buffer[0] };
						int linesize[] = { width * 4 };
						sws_scale(img_convert_ctx, videoFrame->data, videoFrame->linesize, 0, pVideoCodecCtx->height,
						  data, linesize);
						av_free(videoFrame);
						newVideoFrame = true;
					}
					VideoFrame * tmp = new VideoFrame();
					tmp->data.swap(buffer);
					tmp->height = height;
					tmp->width = width;
					tmp->timestamp = time;
					videoQueue.push(tmp);
				}
			}
			if( decodeSize < 0 ) {
				av_free(videoFrame);
				av_free_packet(&packet);
				newVideoFrame = true;
				throw std::runtime_error("cannot decode video frame");
			}

			if(frameFinished) {
				av_free_packet(&packet);
				return;
			}

			av_free_packet(&packet); // TODO: This is repeated in every branch of execution, so it clearly needs RAII wrapping
		} else if(packet.stream_index==audioStream) {
			if( !decodeAudio ) {
				av_free_packet(&packet);
				return;
			}
			int16_t audioFrames[AVCODEC_MAX_AUDIO_FRAME_SIZE];
			int outsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*sizeof(int16_t);
			float time;

			int decodeSize = avcodec_decode_audio2( pAudioCodecCtx, audioFrames, &outsize, packet.data, packet.size);

			// No data decoded
			if( outsize == 0 ) {
				av_free_packet(&packet);
				std::cout << "Empty audio frame" << std::endl;
				return;
			}

			if( decodeSize < 0 ) {
				av_free_packet(&packet);
				throw std::runtime_error("cannot decode audio frame");
			}

			if( (uint64_t)packet.pts != AV_NOPTS_VALUE ) {
				time = av_q2d(pFormatCtx->streams[audioStream]->time_base) * packet.pts;
			} else if( (uint64_t)packet.dts != AV_NOPTS_VALUE ) {
				time = av_q2d(pFormatCtx->streams[audioStream]->time_base) * packet.dts;
			} else {
				time = 0.0;
			}

			std::vector<int16_t> audioFramesResampled(AVCODEC_MAX_AUDIO_FRAME_SIZE);
			int nb_sample = audio_resample(pResampleCtx, &audioFramesResampled[0], audioFrames, outsize/(pAudioCodecCtx->channels*2));

			audioFramesResampled.resize(nb_sample);

			AudioFrame* tmp = new AudioFrame();
			tmp->data.swap(audioFramesResampled);
			tmp->timestamp = time;
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

