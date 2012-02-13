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

#define AUDIO_CHANNELS 2

/*static*/ boost::mutex FFmpeg::s_avcodec_mutex;

FFmpeg::FFmpeg(bool _decodeVideo, bool _decodeAudio, std::string const& _filename, unsigned int rate):
  width(), height(), m_filename(_filename), m_rate(rate), m_quit(), m_running(), m_eof(), m_seekTarget(getNaN()),
  pFormatCtx(), pResampleCtx(), img_convert_ctx(), pVideoCodecCtx(), pAudioCodecCtx(), pVideoCodec(), pAudioCodec(),
  videoStream(-1), audioStream(-1), decodeVideo(_decodeVideo), decodeAudio(_decodeAudio), m_position(),
  m_thread(new boost::thread(boost::ref(*this)))
{}

FFmpeg::~FFmpeg() {
	m_quit = true;
	videoQueue.reset();
	audioQueue.quit();
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
	av_log_set_level(AV_LOG_ERROR);
	if (avformat_open_input(&pFormatCtx, m_filename.c_str(), NULL, NULL)) throw std::runtime_error("Cannot open input file");
	if (av_find_stream_info(pFormatCtx) < 0) throw std::runtime_error("Cannot find stream information");
	pFormatCtx->flags |= AVFMT_FLAG_GENPTS;
	videoStream = -1;
	audioStream = -1;
	// Take the first video/audio streams
	for (unsigned int i=0; i<pFormatCtx->nb_streams; i++) {
		AVCodecContext* cc = pFormatCtx->streams[i]->codec;
		cc->workaround_bugs = FF_BUG_AUTODETECT;
		if (videoStream == -1 && cc->codec_type==AVMEDIA_TYPE_VIDEO) videoStream = i;
		if (audioStream == -1 && cc->codec_type==AVMEDIA_TYPE_AUDIO) audioStream = i;
	}
	if (videoStream == -1 && decodeVideo) throw std::runtime_error("No video stream found");
	if (audioStream == -1 && decodeAudio) throw std::runtime_error("No audio stream found");
	if (decodeVideo) {
		AVCodecContext* cc = pFormatCtx->streams[videoStream]->codec;
		pVideoCodec = avcodec_find_decoder(cc->codec_id);
		if (!pVideoCodec) throw std::runtime_error("Cannot find video codec");
		if (avcodec_open2(cc, pVideoCodec, NULL) < 0) throw std::runtime_error("Cannot open video codec");
		pVideoCodecCtx = cc;
	}
	if (decodeAudio) {
		AVCodecContext* cc = pFormatCtx->streams[audioStream]->codec;
		pAudioCodec = avcodec_find_decoder(cc->codec_id);
		if (!pAudioCodec) throw std::runtime_error("Cannot find audio codec");
		if (avcodec_open2(cc, pAudioCodec, NULL) < 0) throw std::runtime_error("Cannot open audio codec");
		pAudioCodecCtx = cc;
		pResampleCtx = av_audio_resample_init(AUDIO_CHANNELS, cc->channels, m_rate, cc->sample_rate, SAMPLE_FMT_S16, SAMPLE_FMT_S16, 16, 10, 0, 0.8);
		if (!pResampleCtx) throw std::runtime_error("Cannot create resampling context");
		audioQueue.setSamplesPerSecond(AUDIO_CHANNELS * m_rate);
	}
	// Setup software scaling context for YUV to RGB conversion
	if (videoStream != -1 && decodeVideo) {
		width = pVideoCodecCtx->width;
		height = pVideoCodecCtx->height;
		img_convert_ctx = sws_getContext(
		  pVideoCodecCtx->width, pVideoCodecCtx->height, pVideoCodecCtx->pix_fmt,
		  width, height, PIX_FMT_RGB24,
		  SWS_POINT, NULL, NULL, NULL);
	}
}

void FFmpeg::operator()() {
	try { open(); } catch (std::exception const& e) { std::clog << "ffmpeg/error: Failed to open " << m_filename << ": " << e.what() << std::endl; m_quit = true; return; }
	m_running = true;
	audioQueue.setDuration(duration());
	int errors = 0;
	while (!m_quit) {
		try {
			if (audioQueue.wantSeek()) m_seekTarget = 0.0;
			if (m_seekTarget == m_seekTarget) seek_internal();
			decodePacket();
			m_eof = false;
			errors = 0;
		} catch (eof_error&) {
			m_eof = true;
			videoQueue.push(new VideoFrame()); // EOF marker
			boost::thread::sleep(now() + 0.1);
		} catch (std::exception& e) {
			std::clog << "ffmpeg/error: " << m_filename << ": " << e.what() << std::endl;
			if (++errors > 2) { std::clog << "ffmpeg/error: FFMPEG terminating due to multiple errors" << std::endl; m_quit = true; }
		}
	}
	m_running = false;
	m_eof = true;
	audioQueue.setEof();
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

struct ReadFramePacket: public AVPacket {
	AVFormatContext* m_s;
	ReadFramePacket(AVFormatContext* s): m_s(s) {
		if (av_read_frame(s, this) < 0) throw FFmpeg::eof_error();
	}
	~ReadFramePacket() { av_free_packet(this); }
	double time() {
		return uint64_t(dts) == uint64_t(AV_NOPTS_VALUE) ?
		  getNaN() : double(dts) * av_q2d(m_s->streams[stream_index]->time_base);
	}
};

void FFmpeg::decodePacket() {
	ReadFramePacket packet(pFormatCtx);
	int packetSize = packet.size;
	while (packetSize) {
		if (packetSize < 0) throw std::logic_error("negative packet size?!");
		if (m_quit || m_seekTarget == m_seekTarget) return;
		int decodeSize = 0;
		if (decodeVideo && packet.stream_index == videoStream) decodeSize = decodeVideoFrame(packet);
		else if (decodeAudio && packet.stream_index == audioStream) decodeSize = decodeAudioFrame(packet);
		else return;
		packetSize -= decodeSize; // Move forward within the packet
	}
}

int FFmpeg::decodeVideoFrame(ReadFramePacket& packet) {
	struct AVFrameWrapper {
		AVFrame* m_frame;
		AVFrameWrapper(): m_frame(avcodec_alloc_frame()) {
			if (!m_frame) throw std::runtime_error("Unable to allocate AVFrame");
		}
		~AVFrameWrapper() { av_free(m_frame); }
		operator AVFrame*() { return m_frame; }
		AVFrame* operator->() { return m_frame; }
	} videoFrame;

	int frameFinished = 0;
	int decodeSize = avcodec_decode_video2(pVideoCodecCtx, videoFrame, &frameFinished, &packet);
	if (decodeSize < 0) throw std::runtime_error("cannot decode video frame");
	if (frameFinished) {
		// Convert into RGB and scale the data
		int w = (pVideoCodecCtx->width+15)&~15;
		int h = pVideoCodecCtx->height;
		std::vector<uint8_t> buffer(w * h * 3);
		{
			uint8_t* data = &buffer[0];
			int linesize = w * 3;
			sws_scale(img_convert_ctx, videoFrame->data, videoFrame->linesize, 0, h, &data, &linesize);
		}
		if (packet.time() == packet.time()) m_position = packet.time();
		// Construct a new video frame and push it to output queue
		VideoFrame* tmp = new VideoFrame(m_position, w, h);
		tmp->data.swap(buffer);
		videoQueue.push(tmp); // Takes ownership and may block
	}
	return decodeSize;
}

int FFmpeg::decodeAudioFrame(ReadFramePacket& packet) {
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
	} audioFrames(AVCODEC_MAX_AUDIO_FRAME_SIZE);
	
	int outsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*sizeof(int16_t);
	int decodeSize = avcodec_decode_audio3(pAudioCodecCtx, audioFrames, &outsize, &packet);
	if (decodeSize < 0) throw std::runtime_error("cannot decode audio frame");
	if (outsize > 0) {
		// Convert outsize from bytes into number of frames (samples)
		outsize /= sizeof(int16_t) * pAudioCodecCtx->channels;
		std::vector<int16_t> resampled(AVCODEC_MAX_AUDIO_FRAME_SIZE);
		int frames = audio_resample(pResampleCtx, &resampled[0], audioFrames, outsize);
		resampled.resize(frames * AUDIO_CHANNELS);
		// Use timecode from packet if available
		if (packet.time() == packet.time()) m_position = packet.time();
		// Push to output queue (may block)
		audioQueue.push(resampled, m_position);
		// Increment current time
		m_position += double(resampled.size())/double(audioQueue.getSamplesPerSecond());
	}
	return decodeSize;
}

