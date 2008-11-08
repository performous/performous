#include "ffmpeg.hh"

#include "xtime.hh"
#include <iostream>
#include <stdexcept>

extern "C" {
#include <avcodec.h>
#include <avformat.h>
#include <swscale.h>
}

#define USE_FFMPEG_CRASH_RECOVERY
#define FFMPEG_OUTPUT_NUMBER_OF_CHANNELS 2

/*static*/ boost::mutex CFfmpeg::s_avcodec_mutex;

CFfmpeg::CFfmpeg(bool _decodeVideo, bool _decodeAudio, std::string const& _filename, unsigned int rate): m_filename(_filename), m_rate(rate), m_quit() {
	av_register_all();
	videoStream=-1;
	audioStream=-1;
	decodeVideo=_decodeVideo;
	decodeAudio=_decodeAudio;
	open();
}

CFfmpeg::~CFfmpeg() {
	m_quit = true;
	videoQueue.reset();
	audioQueue.reset();
	if (!m_thread) {
		std::cerr << "FFMPEG crashed at some point, decoding " << m_filename << std::endl;
		std::cerr << "Please restart USNG to avoid resource leaks and program crashes!" << std::endl;
		return;
	}
	m_thread->join();
	if( videoStream != -1 && decodeVideo ) avcodec_close(pVideoCodecCtx);
	if( audioStream != -1 && decodeAudio ) {
		boost::mutex::scoped_lock l(s_avcodec_mutex);
		avcodec_close(pAudioCodecCtx);
		audio_resample_close(pResampleCtx);
	}
	av_close_input_file(pFormatCtx);
}

double CFfmpeg::duration() {
	double result=0.;
	if( audioStream != -1 && decodeAudio ) {
		result = pFormatCtx->duration / (AV_TIME_BASE*1.);
	} else if( videoStream != -1 && decodeVideo ) {
		result = pFormatCtx->duration / (AV_TIME_BASE*1.);
	} else {
		result = -1.0;
	}
	return result;
}

void CFfmpeg::open() {
	if (av_open_input_file(&pFormatCtx, m_filename.c_str(), NULL, 0, NULL)) throw std::runtime_error("Cannot open input file");
	if (av_find_stream_info(pFormatCtx) < 0) throw std::runtime_error("Cannot find stream information");
	dump_format(pFormatCtx, 0, m_filename.c_str(), false);
	videoStream = -1;
	audioStream = -1;
	m_seekTarget = std::numeric_limits<double>::quiet_NaN();
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
			boost::mutex::scoped_lock l(s_avcodec_mutex);
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
			pResampleCtx = audio_resample_init(FFMPEG_OUTPUT_NUMBER_OF_CHANNELS, pAudioCodecCtx->channels, m_rate, pAudioCodecCtx->sample_rate);
			if (!pResampleCtx) throw std::runtime_error("Cannot create resampling context");
			std::cout << "Resampling audio from " << pAudioCodecCtx->channels << " channel(s) at " << pAudioCodecCtx->sample_rate << "Hz";
			std::cout << " to " << FFMPEG_OUTPUT_NUMBER_OF_CHANNELS << " channels at " << m_rate << "Hz" << std::endl;
			audioQueue.setSamplesPerSecond(FFMPEG_OUTPUT_NUMBER_OF_CHANNELS * m_rate);
		}
	} catch (std::runtime_error& e) {
		// TODO: clean memory
		std::cerr << "Failed to load audio (" <<  e.what() << ")" <<std::endl;
		audioStream = -1;
	}
	// Setup software scaling context
	if( videoStream != -1 && decodeVideo ) {
		width = pVideoCodecCtx->width;
		height = pVideoCodecCtx->height;
		img_convert_ctx = sws_getContext(
		  width, height, pVideoCodecCtx->pix_fmt,
		  width, height, PIX_FMT_RGB24,
		  SWS_BICUBIC, NULL, NULL, NULL);
	}
	m_thread.reset(new boost::thread(boost::ref(*this)));
}

#ifdef USE_FFMPEG_CRASH_RECOVERY
#include <boost/thread/tss.hpp>
#include <unistd.h>
#include <csignal>

namespace {
	boost::thread_specific_ptr<CFfmpeg*> ffmpeg_ptr;
}

extern "C" void usng_ffmpeg_crash_hack(int) {
	if (ffmpeg_ptr.get()) { (*ffmpeg_ptr)->crash(); sleep(1000000000); }
	else std::abort(); // Crash from another thread
}
#endif

void CFfmpeg::operator()() {
#ifdef USE_FFMPEG_CRASH_RECOVERY
	// A hack to avoid ffmpeg crashing USNG :)
	ffmpeg_ptr.reset(new CFfmpeg*);
	*ffmpeg_ptr = this;
	std::signal(SIGABRT, usng_ffmpeg_crash_hack);
	std::signal(SIGSEGV, usng_ffmpeg_crash_hack);
#endif
	int errors = 0;
	while (!m_quit) {
		try {
			if (m_seekTarget == m_seekTarget) seek_internal();
			decodeNextFrame();
			m_eof = false;
			errors = 0;
		} catch (eof_error&) {
			m_eof = true;
			audioQueue.push(new AudioFrame()); // EOF marker
			boost::thread::sleep(now() + 0.1);
		} catch (std::exception& e) {
			std::cerr << "FFMPEG error: " << e.what() << std::endl;
			if (++errors > 2) break;
		}
	}
}

void CFfmpeg::seek(double time) {
	m_seekTarget = time;
	videoQueue.reset(); audioQueue.reset(); // Empty these to unblock the internals in case buffers were full
	while (m_seekTarget == m_seekTarget) boost::thread::sleep(now() + 0.01); // Wait until seek_internal is done
}

void CFfmpeg::seek_internal() {
	audioQueue.reset();
	videoQueue.reset();
	av_seek_frame(pFormatCtx, -1, m_seekTarget * AV_TIME_BASE, 0);
	m_seekTarget = std::numeric_limits<double>::quiet_NaN();
}

void CFfmpeg::decodeNextFrame() {
	struct ReadFramePacket: public AVPacket {
		AVFormatContext* m_s;
		ReadFramePacket(AVFormatContext* s): m_s(s) {
			if (av_read_frame(s, this) < 0) throw eof_error();
		}
		~ReadFramePacket() { av_free_packet(this); }
		double time() {
			if( uint64_t(dts) == uint64_t(AV_NOPTS_VALUE) )
				return double(-1);
			else
				return double(dts) * av_q2d(m_s->streams[stream_index]->time_base);
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
					if( packet.time() != -1 ) {
						m_position = packet.time();
					}
					tmp->timestamp = m_position;
					videoQueue.push(tmp);
				}
				if (m_quit) return;
			}
		} else if(packet.stream_index==audioStream) {
			if (!decodeAudio) return;
			int packetsize = packet.size;
			uint8_t *packetbuffer = packet.data;

			while( packetsize ) {
				int16_t audioFrames[AVCODEC_MAX_AUDIO_FRAME_SIZE];
				int outsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*sizeof(int16_t);
				int decodeSize = avcodec_decode_audio2( pAudioCodecCtx, audioFrames, &outsize, packetbuffer, packetsize);

				// No data decoded
				if (outsize == 0) {
					break;
				}

				if (decodeSize < 0) throw std::runtime_error("cannot decode audio frame");
	
				outsize /= (sizeof(int16_t)*pAudioCodecCtx->channels); // Outsize is givent in bytes, we want a number of sample
				std::vector<int16_t> audioFramesResampled(AVCODEC_MAX_AUDIO_FRAME_SIZE);
				int nb_sample = audio_resample(pResampleCtx, &audioFramesResampled[0], audioFrames, outsize);
				nb_sample *= FFMPEG_OUTPUT_NUMBER_OF_CHANNELS; // We have multiple channels
	
				audioFramesResampled.resize(nb_sample);

				AudioFrame* tmp = new AudioFrame();
				tmp->data.swap(audioFramesResampled);
				//tmp->data = std::vector<int16_t>(audioFrames, audioFrames + outsize / sizeof(int16_t));
				if( packet.time() != -1 ) {
					m_position = packet.time();
				} else {
					m_position += double(tmp->data.size())/double(audioQueue.getSamplesPerSecond());
				}
				tmp->timestamp = m_position;
				audioQueue.push(tmp);

				// advance into the buffer
				packetsize -= decodeSize;
				packetbuffer += decodeSize;
			}
			// Audio frame are always finished
			frameFinished = 1;
			if (m_quit) return;
		}
	}
}

