#include "ffmpeg.hpp"

#ifdef USE_FFMPEG_VIDEO

#include <stdexcept>
#include <iostream>
#include <stdio.h>

CFfmpeg::CFfmpeg(bool _decodeVideo, bool _decodeAudio, std::string const& _filename): m_quit() {
	av_register_all();
	videoStream=-1;
	audioStream=-1;
	decodeVideo=_decodeVideo;
	decodeAudio=_decodeAudio;
	open(_filename.c_str());
	m_thread.reset(new boost::thread(boost::ref(*this)));
}

CFfmpeg::~CFfmpeg() {
	m_quit = true;
	videoQueue.reset();
	// audioQueue.reset();
	m_thread->join();
	if( videoStream != -1 && decodeVideo ) avcodec_close(pVideoCodecCtx);
	if( audioStream != -1 && decodeAudio ) {
		avcodec_close(pAudioCodecCtx);
		audio_resample_close(pResampleCtx);
	}
	av_close_input_file(pFormatCtx);
}

void CFfmpeg::open(const char* _filename) {
	if (av_open_input_file(&pFormatCtx, _filename, NULL, 0, NULL)) throw std::runtime_error("Cannot open input file");
	if (av_find_stream_info(pFormatCtx) < 0) throw std::runtime_error("Cannot find stream information");
	dump_format(pFormatCtx, 0, _filename, false);
	videoStream = -1;
	audioStream = -1;
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
			pVideoCodecCtx = pFormatCtx->streams[videoStream]->codec;
			pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
			if (!pVideoCodec) throw std::runtime_error("Cannot find video codec");
			if (avcodec_open(pVideoCodecCtx, pVideoCodec) < 0) throw std::runtime_error("Cannot open video codec");
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
			if (!pAudioCodec) throw std::runtime_error("Cannot find audio codec");
			if (avcodec_open(pAudioCodecCtx, pAudioCodec) < 0) throw std::runtime_error("Cannot open audio codec");
			pResampleCtx = audio_resample_init(2,pAudioCodecCtx->channels,48000,pAudioCodecCtx->sample_rate);
			if (!pResampleCtx) throw std::runtime_error("Cannot create resampling context");
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
	  w, h, PIX_FMT_RGB24,
	  SWS_BICUBIC, NULL, NULL, NULL);
}

void CFfmpeg::operator()() {
	int errors = 0;
	while (!m_quit) {
		try {
			decodeNextFrame();
			errors = 0;
		} catch (std::exception& e) {
			std::cerr << "ffmpeg error: " << e.what() << std::endl;
			if (++errors > 2) break;
		}
	}
}

void CFfmpeg::decodeNextFrame() {
	struct ReadFramePacket: public AVPacket {
		AVFormatContext* m_s;
		ReadFramePacket(AVFormatContext* s): m_s(s) {
			if (av_read_frame(s, this) < 0) throw std::runtime_error("End of file");
		}
		~ReadFramePacket() { av_free_packet(this); }
		double time() {
			double t = 0.0;
			if (uint64_t(pts) != AV_NOPTS_VALUE) t = pts;
			else if (uint64_t(dts) != AV_NOPTS_VALUE ) t = dts;
			return t ? t * av_q2d(m_s->streams[stream_index]->time_base) : 0.0;
		}
	};

	struct AVFrameWrapper {
		AVFrame* m_frame;
		AVFrameWrapper(): m_frame(avcodec_alloc_frame()) {
			if (!m_frame) throw std::runtime_error("Unable to allocate AVFrame");
		}
		~AVFrameWrapper() { av_free(m_frame); }
		operator AVFrame*() { return m_frame; }
		AVFrame* operator->() { return m_frame; }
	} videoFrame;

	int frameFinished=0;
	while (!frameFinished) {
		ReadFramePacket packet(pFormatCtx);
		if (packet.stream_index == videoStream) {
			if (!decodeVideo) return;
			uint8_t* packetData = packet.data;
			int packetSize = packet.size;
			while (packetSize) {
				if (packetSize < 0) throw std::logic_error("negative video packet size?!");
				int decodeSize = avcodec_decode_video(pVideoCodecCtx, videoFrame, &frameFinished, packetData, packetSize);
				if (decodeSize < 0) throw std::runtime_error("cannot decode video frame");
				packetData += decodeSize;
				packetSize -= decodeSize;
				if (frameFinished) {
					// Convert into RGB and scale the data
					int w = pVideoCodecCtx->width;
					int h = pVideoCodecCtx->height;
					std::vector<uint8_t> buffer(w * h * 3);
					{
						uint8_t* data = &buffer[0];
						int linesize = w * 3;
						sws_scale(img_convert_ctx, videoFrame->data, videoFrame->linesize, 0, h, &data, &linesize);
					}
					VideoFrame* tmp = new VideoFrame();
					tmp->data.swap(buffer);
					tmp->height = h;
					tmp->width = w;
					tmp->timestamp = packet.time();
					static double prev = 0.0;
					std::cout << "Video frame duration " << tmp->timestamp - prev << std::endl;
					prev = tmp->timestamp;
					videoQueue.push(tmp);
				}
				if (m_quit) return;
			}
		} else if(packet.stream_index==audioStream) {
			if (!decodeAudio) return;
			int16_t audioFrames[AVCODEC_MAX_AUDIO_FRAME_SIZE];
			int outsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*sizeof(int16_t);
			int decodeSize = avcodec_decode_audio2( pAudioCodecCtx, audioFrames, &outsize, packet.data, packet.size);

			// No data decoded
			if (outsize == 0) {
				std::cout << "Empty audio frame" << std::endl;
				return;
			}

			if (decodeSize < 0) throw std::runtime_error("cannot decode audio frame");

			std::vector<int16_t> audioFramesResampled(AVCODEC_MAX_AUDIO_FRAME_SIZE);
			int nb_sample = audio_resample(pResampleCtx, &audioFramesResampled[0], audioFrames, outsize/(pAudioCodecCtx->channels*2));

			audioFramesResampled.resize(nb_sample);

			AudioFrame* tmp = new AudioFrame();
			tmp->data.swap(audioFramesResampled);
			tmp->timestamp = packet.time();
			audioQueue.push(tmp);
			if (m_quit) return;
		}
	}
}

#endif
