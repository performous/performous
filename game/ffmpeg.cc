#include "ffmpeg.hh"

#include "config.hh"
#include "util.hh"
#include "xtime.hh"
#include <iostream>
#include <stdexcept>

extern "C" {
#include AVCODEC_INCLUDE
#include AVFORMAT_INCLUDE
#include SWSCALE_INCLUDE
}

#define USE_FFMPEG_CRASH_RECOVERY
#define AUDIO_CHANNELS 2

/*static*/ boost::mutex CFfmpeg::s_avcodec_mutex;

CFfmpeg::CFfmpeg(bool _decodeVideo, bool _decodeAudio, std::string const& _filename, unsigned int rate):
  m_filename(_filename), m_rate(rate), m_quit(),
  pFormatCtx(), pResampleCtx(), img_convert_ctx(), pVideoCodecCtx(), pAudioCodecCtx(), pVideoCodec(), pAudioCodec()
{
	videoStream=-1;
	audioStream=-1;
	decodeVideo=_decodeVideo;
	decodeAudio=_decodeAudio;
	m_thread.reset(new boost::thread(boost::ref(*this)));
}

CFfmpeg::~CFfmpeg() {
	m_quit = true;
	videoQueue.reset();
	audioQueue.reset();
	if (!m_thread) {
		std::cerr << "FFMPEG crashed at some point, decoding " << m_filename << std::endl;
		std::cerr << "Please restart Performous to avoid resource leaks and program crashes!" << std::endl;
		return;
	}
	m_thread->join();
	// TODO: use RAII for freeing resources (to prevent memory leaks)
	boost::mutex::scoped_lock l(s_avcodec_mutex); // avcodec_close is not thread-safe
	if (pVideoCodecCtx) avcodec_close(pVideoCodecCtx);
	if (pAudioCodecCtx) avcodec_close(pAudioCodecCtx);
	if (pResampleCtx) audio_resample_close(pResampleCtx);
	if (pFormatCtx) av_close_input_file(pFormatCtx);
}

double CFfmpeg::duration() {
	if (pFormatCtx) return pFormatCtx->duration / double(AV_TIME_BASE);
	return getNaN();
}

void CFfmpeg::open() {
	av_register_all();
	av_log_set_level(AV_LOG_QUIET);
	if (av_open_input_file(&pFormatCtx, m_filename.c_str(), NULL, 0, NULL)) throw std::runtime_error("Cannot open input file");
	if (av_find_stream_info(pFormatCtx) < 0) throw std::runtime_error("Cannot find stream information");
	videoStream = -1;
	audioStream = -1;
	m_seekTarget = getNaN();
	// Take the first video/audio streams
	for (unsigned int i=0; i<pFormatCtx->nb_streams; i++) {
		if (videoStream == -1 && pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) videoStream = i;
		if (audioStream == -1 && pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO) audioStream = i;
	}
	if (videoStream == -1 && decodeVideo) throw std::runtime_error("No video stream found");
	if (audioStream == -1 && decodeAudio) throw std::runtime_error("No audio stream found");
	if (decodeVideo) {
		pVideoCodecCtx = pFormatCtx->streams[videoStream]->codec;
		pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
		if (!pVideoCodec) throw std::runtime_error("Cannot find video codec");
		boost::mutex::scoped_lock l(s_avcodec_mutex);
		if (avcodec_open(pVideoCodecCtx, pVideoCodec) < 0) throw std::runtime_error("Cannot open video codec");
	}
	if (decodeAudio) {
		pAudioCodecCtx=pFormatCtx->streams[audioStream]->codec;
		pAudioCodec=avcodec_find_decoder(pAudioCodecCtx->codec_id);
		if (!pAudioCodec) throw std::runtime_error("Cannot find audio codec");
		if (avcodec_open(pAudioCodecCtx, pAudioCodec) < 0) throw std::runtime_error("Cannot open audio codec");
		pResampleCtx = audio_resample_init(AUDIO_CHANNELS, pAudioCodecCtx->channels, m_rate, pAudioCodecCtx->sample_rate);
		if (!pResampleCtx) throw std::runtime_error("Cannot create resampling context");
		audioQueue.setSamplesPerSecond(AUDIO_CHANNELS * m_rate);
	}
	// Setup software scaling context for YUV to RGB conversion
	if (videoStream != -1 && decodeVideo) {
		width = pVideoCodecCtx->width;
		height = pVideoCodecCtx->height;
		img_convert_ctx = sws_getContext(
		  width, height, pVideoCodecCtx->pix_fmt,
		  width, height, PIX_FMT_RGB24,
		  SWS_POINT, NULL, NULL, NULL);
	}
}

#ifdef USE_FFMPEG_CRASH_RECOVERY
#include <boost/thread/tss.hpp>
#include <unistd.h>
#include <csignal>

namespace {
	boost::thread_specific_ptr<CFfmpeg*> ffmpeg_ptr;
	typedef void (*sighandler)(int);
	sighandler sigabrt, sigsegv;
}

extern "C" void usng_ffmpeg_crash_hack(int sig) {
	if (ffmpeg_ptr.get()) {
		signal(SIGABRT, sigabrt);
		signal(SIGSEGV, sigsegv);
		(*ffmpeg_ptr)->crash(); sleep(1000000000);
	} // Uh-oh, FFMPEG goes again; wait here until eternity
	sighandler h = (sig == SIGABRT ? sigabrt : sigsegv);
	if (h && h != usng_ffmpeg_crash_hack) h(sig); // From another thread, call original handler
	std::abort();
}
#endif

void CFfmpeg::operator()() {
#ifdef USE_FFMPEG_CRASH_RECOVERY
	// A hack to avoid ffmpeg crashing the program :)
	ffmpeg_ptr.reset(new CFfmpeg*);
	*ffmpeg_ptr = this;
	sigabrt = std::signal(SIGABRT, usng_ffmpeg_crash_hack);
	sigsegv = std::signal(SIGSEGV, usng_ffmpeg_crash_hack);
#endif
	try { open(); } catch (std::exception const& e) { std::cerr << "FFMPEG failed to open " << m_filename << ": " << e.what() << std::endl; }
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
	while (m_thread && m_seekTarget == m_seekTarget) boost::thread::sleep(now() + 0.01); // Wait until seek_internal is done
}

void CFfmpeg::seek_internal() {
	audioQueue.reset();
	videoQueue.reset();
	av_seek_frame(pFormatCtx, -1, m_seekTarget * AV_TIME_BASE, 0);
	m_seekTarget = getNaN();
}

void CFfmpeg::decodeNextFrame() {
	struct ReadFramePacket: public AVPacket {
		AVFormatContext* m_s;
		ReadFramePacket(AVFormatContext* s): m_s(s) {
			if (av_read_frame(s, this) < 0) throw eof_error();
		}
		~ReadFramePacket() { av_free_packet(this); }
		double time() {
			return uint64_t(dts) == uint64_t(AV_NOPTS_VALUE) ?
			  getNaN() : double(dts) * av_q2d(m_s->streams[stream_index]->time_base);
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
		int packetSize = packet.size;
		uint8_t *packetData = packet.data;
		if (decodeVideo && packet.stream_index == videoStream) {
			while (packetSize) {
				if (packetSize < 0) throw std::logic_error("negative video packet size?!");
				int decodeSize = avcodec_decode_video(pVideoCodecCtx, videoFrame, &frameFinished, packetData, packetSize);
				if (decodeSize < 0) throw std::runtime_error("cannot decode video frame");
				// Move forward within the packet
				packetSize -= decodeSize;
				packetData += decodeSize;
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
					if ( !std::isnan(packet.time()) ) m_position = packet.time();
					VideoFrame* tmp = new VideoFrame(m_position, w, h);
					tmp->data.swap(buffer);
					videoQueue.push(tmp);
				}
				if (m_quit) return;
			}
		} else if (decodeAudio && packet.stream_index==audioStream) {
			while (packetSize) {
				if (packetSize < 0) throw std::logic_error("negative audio packet size?!");
				int16_t audioFrames[AVCODEC_MAX_AUDIO_FRAME_SIZE];
				int outsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*sizeof(int16_t);
				int decodeSize = avcodec_decode_audio2( pAudioCodecCtx, audioFrames, &outsize, packetData, packetSize);
				// Handle special cases
				if (decodeSize == 0) break;
				if (outsize == 0) continue;
				if (decodeSize < 0) throw std::runtime_error("cannot decode audio frame");
				// Move forward within the packet
				packetSize -= decodeSize;
				packetData += decodeSize;
				// Convert outsize from bytes into number of frames (samples)
				outsize /= sizeof(int16_t) * pAudioCodecCtx->channels;
				int16_t resampled[AVCODEC_MAX_AUDIO_FRAME_SIZE];
				int frames = audio_resample(pResampleCtx, resampled, audioFrames, outsize);
				// Construct AudioFrame and add it to the queue
				AudioFrame* tmp = new AudioFrame();
				std::copy(resampled, resampled + frames * AUDIO_CHANNELS, std::back_inserter(tmp->data));
				if (!std::isnan(packet.time()) ) m_position = packet.time();
				else m_position += double(tmp->data.size())/double(audioQueue.getSamplesPerSecond());
				tmp->timestamp = m_position;
				audioQueue.push(tmp);
				if (m_quit) return;
			}
			// Audio frames are always finished
			frameFinished = 1;
		}
	}
}

