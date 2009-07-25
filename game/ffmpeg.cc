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

/*static*/ boost::mutex FFmpeg::s_avcodec_mutex;

FFmpeg::FFmpeg(bool _decodeVideo, bool _decodeAudio, std::string const& _filename, unsigned int rate):
  m_filename(_filename), m_rate(rate), m_quit(), m_running(), m_eof(), m_seekTarget(getNaN()),
  pFormatCtx(), pResampleCtx(), img_convert_ctx(), pVideoCodecCtx(), pAudioCodecCtx(), pVideoCodec(), pAudioCodec(),
  videoStream(-1), audioStream(-1), decodeVideo(_decodeVideo), decodeAudio(_decodeAudio), m_position(),
  m_thread(new boost::thread(boost::ref(*this)))
{}

FFmpeg::~FFmpeg() {
	m_quit = true;
	videoQueue.reset();
	audioQueue.quit();
	if (!m_thread) {
		std::cerr << "FFMPEG crashed at some point, decoding " << m_filename << std::endl;
		std::cerr << "Please restart Performous to avoid resource leaks and program crashes!" << std::endl;
		return;
	}
	m_thread->join();
	// TODO: use RAII for freeing resources (to prevent memory leaks)
	boost::mutex::scoped_lock l(s_avcodec_mutex); // avcodec_close is not thread-safe
	if (pResampleCtx) audio_resample_close(pResampleCtx);
	if (pAudioCodecCtx) avcodec_close(pAudioCodecCtx);
	if (pVideoCodecCtx) avcodec_close(pVideoCodecCtx);
	if (pFormatCtx) av_close_input_file(pFormatCtx);
}

double FFmpeg::duration() const {
	double d = m_running ? pFormatCtx->duration / double(AV_TIME_BASE) : getNaN();
	return d >= 0.0 ? d : getInf();
}

void FFmpeg::open() {
	boost::mutex::scoped_lock l(s_avcodec_mutex);
	av_register_all();
	//av_log_set_level(AV_LOG_DEBUG);
	if (av_open_input_file(&pFormatCtx, m_filename.c_str(), NULL, 0, NULL)) throw std::runtime_error("Cannot open input file");
	if (av_find_stream_info(pFormatCtx) < 0) throw std::runtime_error("Cannot find stream information");
	pFormatCtx->flags |= AVFMT_FLAG_GENPTS;
	videoStream = -1;
	audioStream = -1;
	// Take the first video/audio streams
	for (unsigned int i=0; i<pFormatCtx->nb_streams; i++) {
		AVCodecContext* cc = pFormatCtx->streams[i]->codec;
		cc->workaround_bugs = FF_BUG_AUTODETECT;
		if (videoStream == -1 && cc->codec_type==CODEC_TYPE_VIDEO) videoStream = i;
		if (audioStream == -1 && cc->codec_type==CODEC_TYPE_AUDIO) audioStream = i;
	}
	if (videoStream == -1 && decodeVideo) throw std::runtime_error("No video stream found");
	if (audioStream == -1 && decodeAudio) throw std::runtime_error("No audio stream found");
	if (decodeVideo) {
		AVCodecContext* cc = pFormatCtx->streams[videoStream]->codec;
		pVideoCodec = avcodec_find_decoder(cc->codec_id);
		if (!pVideoCodec) throw std::runtime_error("Cannot find video codec");
		if (avcodec_open(cc, pVideoCodec) < 0) throw std::runtime_error("Cannot open video codec");
		pVideoCodecCtx = cc;
	}
	if (decodeAudio) {
		AVCodecContext* cc = pFormatCtx->streams[audioStream]->codec;
		pAudioCodec = avcodec_find_decoder(cc->codec_id);
		// Awesome HACK for AAC to work (unfortunately libavformat fails to decode this information from MPEG ADTS)
		if (pAudioCodec->name == std::string("aac") && cc->extradata_size == 0) {
			cc->extradata = static_cast<uint8_t*>(malloc(2));
			unsigned profile = 1; // 0 = MAIN, 1 = LC/LOW
			unsigned srate_idx = 3; // 3 = 48000 Hz
			unsigned channels = 2; // Just the number of channels
			cc->extradata[0] = ((profile + 1) << 3) | ((srate_idx & 0xE) >> 1);
			cc->extradata[1] = ((srate_idx & 0x1) << 7) | (channels << 3);
			cc->extradata_size = 2;
			cc->sample_rate = 48000;
			cc->channels = 2;
		}
		if (!pAudioCodec) throw std::runtime_error("Cannot find audio codec");
		if (avcodec_open(cc, pAudioCodec) < 0) throw std::runtime_error("Cannot open audio codec");
		pAudioCodecCtx = cc;
#if LIBAVCODEC_VERSION_INT > ((52<<16)+(12<<8)+0)
		pResampleCtx = av_audio_resample_init(AUDIO_CHANNELS, cc->channels, m_rate, cc->sample_rate,
			SAMPLE_FMT_S16, SAMPLE_FMT_S16, 16, 10, 0, 0.8);
#else
		pResampleCtx = audio_resample_init(AUDIO_CHANNELS, cc->channels, m_rate, cc->sample_rate);
#endif
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
#include <csignal>

namespace {
	boost::thread_specific_ptr<FFmpeg*> ffmpeg_ptr;
	typedef void (*sighandler)(int);
	sighandler sigabrt, sigsegv;
}

extern "C" void performous_ffmpeg_crash_hack(int sig) {
	if (ffmpeg_ptr.get()) {
		std::signal(SIGABRT, sigabrt);
		std::signal(SIGSEGV, sigsegv);
		(*ffmpeg_ptr)->crash();
		while (1) boost::thread::sleep(now() + 1000.0);
	} // Uh-oh, FFMPEG goes again; wait here until eternity
	sighandler h = (sig == SIGABRT ? sigabrt : sigsegv);
	if (h && h != performous_ffmpeg_crash_hack) h(sig); // From another thread, call original handler
	std::abort();
}
#endif

void FFmpeg::operator()() {
#ifdef USE_FFMPEG_CRASH_RECOVERY
	// A hack to avoid ffmpeg crashing the program :)
	ffmpeg_ptr.reset(new FFmpeg*);
	*ffmpeg_ptr = this;
	sigabrt = std::signal(SIGABRT, performous_ffmpeg_crash_hack);
	sigsegv = std::signal(SIGSEGV, performous_ffmpeg_crash_hack);
#endif
	try { open(); } catch (std::exception const& e) { std::cerr << "FFMPEG failed to open " << m_filename << ": " << e.what() << std::endl; m_quit = true; }
	m_running = true;
	audioQueue.setDuration(duration());
	int errors = 0;
	while (!m_quit) {
		try {
			if (audioQueue.wantSeek()) m_seekTarget = 0.0;
			if (m_seekTarget == m_seekTarget) seek_internal();
			decodeNextFrame();
			m_eof = false;
			errors = 0;
		} catch (eof_error&) {
			m_eof = true;
			audioQueue.push(new AudioFrame()); // EOF marker
			videoQueue.push(new VideoFrame()); // EOF marker
			boost::thread::sleep(now() + 0.1);
		} catch (std::exception& e) {
			std::cerr << "FFMPEG error: " << e.what() << std::endl;
			if (++errors > 2) { std::cerr << "FFMPEG terminating due to errors" << std::endl; m_quit = true; }
		}
	}
	m_running = false;
	m_eof = true;
	audioQueue.push(new AudioFrame()); // EOF marker
	videoQueue.push(new VideoFrame()); // EOF marker
}

void FFmpeg::seek(double time, bool wait) {
	m_seekTarget = time;
	videoQueue.reset(); audioQueue.reset(); // Empty these to unblock the internals in case buffers were full
	if (wait) while (!m_quit && m_seekTarget == m_seekTarget) boost::thread::sleep(now() + 0.01);
}

void FFmpeg::seek_internal() {
	videoQueue.reset();
	audioQueue.reset();
	int flags = 0;
	if (m_seekTarget < position()) flags |= AVSEEK_FLAG_BACKWARD;
	int stream = -1;
	if (decodeVideo) stream = videoStream;
	if (decodeAudio) stream = audioStream;
	int64_t target = m_seekTarget * AV_TIME_BASE;
	const AVRational time_base_q = { 1, AV_TIME_BASE };  // AV_TIME_BASE_Q is the same thing with C99 struct literal (not supported by MSVC)
	if (stream != -1) target = av_rescale_q(target, time_base_q, pFormatCtx->streams[stream]->time_base);
	av_seek_frame(pFormatCtx, stream, target, flags);
	m_seekTarget = getNaN(); // Signal that seeking is done
}

void FFmpeg::decodeNextFrame() {
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

	class AudioBuffer {
	  public:
		AudioBuffer(size_t _size): m_buffer((int16_t*)av_malloc(_size*sizeof(int16_t))) {
			if (!m_buffer) throw std::runtime_error("Unable to allocate AudioBuffer");
		}
		~AudioBuffer() { av_free(m_buffer); }
		operator  int16_t*() { return m_buffer; }
		int16_t* operator->() { return m_buffer; }
	  private:
		int16_t* m_buffer;
	};

	int frameFinished=0;
	while (!frameFinished) {
		ReadFramePacket packet(pFormatCtx);
		int packetSize = packet.size;
		uint8_t *packetData = packet.data;
		if (decodeVideo && packet.stream_index == videoStream) {
			while (packetSize) {
				if (packetSize < 0) throw std::logic_error("negative video packet size?!");
				if (m_quit || m_seekTarget == m_seekTarget) return;
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
					if (packet.time() == packet.time()) m_position = packet.time();
					VideoFrame* tmp = new VideoFrame(m_position, w, h);
					tmp->data.swap(buffer);
					videoQueue.push(tmp);
				}
			}
		} else if (decodeAudio && packet.stream_index==audioStream) {
			while (packetSize) {
				if (packetSize < 0) throw std::logic_error("negative audio packet size?!");
				if (m_quit || m_seekTarget == m_seekTarget) return;
				AudioBuffer audioFrames(AVCODEC_MAX_AUDIO_FRAME_SIZE);
				int outsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*sizeof(int16_t);
				int decodeSize = avcodec_decode_audio2(pAudioCodecCtx, audioFrames, &outsize, packetData, packetSize);
				// Handle special cases
				if (decodeSize == 0) break;
				if (outsize == 0) continue;
				if (decodeSize < 0) throw std::runtime_error("cannot decode audio frame");
				// Move forward within the packet
				packetSize -= decodeSize;
				packetData += decodeSize;
				// Convert outsize from bytes into number of frames (samples)
				outsize /= sizeof(int16_t) * pAudioCodecCtx->channels;
				std::vector<int16_t> resampled(AVCODEC_MAX_AUDIO_FRAME_SIZE);
				int frames = audio_resample(pResampleCtx, &resampled[0], audioFrames, outsize);
				// Construct AudioFrame and add it to the queue
				AudioFrame* tmp = new AudioFrame();
				std::copy(resampled.begin(), resampled.begin() + frames * AUDIO_CHANNELS, std::back_inserter(tmp->data));
				if (packet.time() == packet.time()) m_position = packet.time();
				else m_position += double(tmp->data.size())/double(audioQueue.getSamplesPerSecond());
				tmp->timestamp = m_position;
				audioQueue.push(tmp);
			}
			// Audio frames are always finished
			frameFinished = 1;
		}
	}
}

