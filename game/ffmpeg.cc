#include "ffmpeg.hh"

#include "chrono.hh"
#include "config.hh"
#include "util.hh"
#include <memory>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <thread>

extern "C" {
#include AVCODEC_INCLUDE
#include AVFORMAT_INCLUDE
#include SWSCALE_INCLUDE
#include AVRESAMPLE_INCLUDE
#include AVUTIL_INCLUDE
#include AVUTIL_OPT_INCLUDE
#include AVUTIL_MATH_INCLUDE
#include AVUTIL_ERROR_INCLUDE
}

#if (LIBAVCODEC_VERSION_INT) < (AV_VERSION_INT(52,94,3))
#	define AV_SAMPLE_FMT_S16 SAMPLE_FMT_S16
#endif

// Some versions of libav does not contain this definition.
#ifndef AV_ERROR_MAX_STRING_SIZE
#	define AV_ERROR_MAX_STRING_SIZE 64
#endif

#define AUDIO_CHANNELS 2

/*static*/ boost::mutex FFmpeg::s_avcodec_mutex;

namespace {
	std::string ffversion(unsigned ver) {
		unsigned major = ver >> 16;
		unsigned minor = (ver >> 8) & 0xFF;
		unsigned micro = ver & 0xFF;
		std::ostringstream oss;
		oss << major << "." << minor << "." << micro << (micro >= 100 ? "(ff)" : "(lav)");
		return oss.str();
	}
}


FFmpeg::FFmpeg(fs::path const& _filename, unsigned int rate):
  width(), height(), m_filename(_filename), m_rate(rate), m_quit(),
  m_seekTarget(getNaN()), m_position(), m_duration(), m_streamId(-1),
  m_mediaType(rate ? AVMEDIA_TYPE_AUDIO : AVMEDIA_TYPE_VIDEO),
  m_formatContext(), m_codecContext(), m_resampleContext(), m_swsContext(),
  m_thread(new boost::thread(boost::ref(*this)))
{
	static bool versionChecked = false;
	if (!versionChecked) {
		versionChecked = true;
		bool matches =
		  LIBAVUTIL_VERSION_INT == avutil_version() &&
		  LIBAVCODEC_VERSION_INT == avcodec_version() &&
		  LIBAVFORMAT_VERSION_INT == avformat_version() &&
		  LIBSWSCALE_VERSION_INT == swscale_version();
		if (matches) {
			std::clog << "ffmpeg/info: "
			  " avutil:" + ffversion(LIBAVUTIL_VERSION_INT) +
			  " avcodec:" + ffversion(LIBAVCODEC_VERSION_INT) +
			  " avformat:" + ffversion(LIBAVFORMAT_VERSION_INT) +
			  " avresample:" + ffversion(LIBAVRESAMPLE_VERSION_INT) +
			  " swscale:" + ffversion(LIBSWSCALE_VERSION_INT)
			  << std::endl;
		} else {
			std::clog << "ffmpeg/error: header/lib version mismatch:"
			  " avutil:" + ffversion(LIBAVUTIL_VERSION_INT) + "/" + ffversion(avutil_version()) +
			  " avcodec:" + ffversion(LIBAVCODEC_VERSION_INT) + "/" + ffversion(avcodec_version()) +
			  " avformat:" + ffversion(LIBAVFORMAT_VERSION_INT) + "/" + ffversion(avformat_version()) +
			  " avresample:" + ffversion(LIBAVRESAMPLE_VERSION_INT) + "/" + ffversion(avresample_version()) +
			  " swscale:" + ffversion(LIBSWSCALE_VERSION_INT) + "/" + ffversion(swscale_version())
			  << std::endl;
		}
	}
}

FFmpeg::~FFmpeg() {
	m_quit = true;
	videoQueue.reset();
	audioQueue.quit();
	m_thread->join();
}

void FFmpeg::open() {
	boost::mutex::scoped_lock l(s_avcodec_mutex);
	av_register_all();
	av_log_set_level(AV_LOG_ERROR);
	if (avformat_open_input(&m_formatContext, m_filename.string().c_str(), nullptr, nullptr)) throw std::runtime_error("Cannot open input file");
	if (avformat_find_stream_info(m_formatContext, nullptr) < 0) throw std::runtime_error("Cannot find stream information");
	m_formatContext->flags |= AVFMT_FLAG_GENPTS;
	// Find a track and open the codec
	AVCodec* codec = nullptr;
	m_streamId = av_find_best_stream(m_formatContext, (AVMediaType)m_mediaType, -1, -1, &codec, 0);
	if (m_streamId < 0) throw std::runtime_error("No suitable track found");

#if (LIBAVCODEC_VERSION_INT) >= (AV_VERSION_INT(57,0,0))
	AVCodec* pCodec = avcodec_find_decoder(m_formatContext->streams[m_streamId]->codecpar->codec_id);
	AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
	avcodec_parameters_to_context(pCodecCtx, m_formatContext->streams[m_streamId]->codecpar);
	if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) throw std::runtime_error("Cannot open codec");
	pCodecCtx->workaround_bugs = FF_BUG_AUTODETECT;
	m_codecContext = pCodecCtx;
#else
	AVCodecContext* cc = m_formatContext->streams[m_streamId]->codec;
	if (avcodec_open2(cc, codec, nullptr) < 0) throw std::runtime_error("Cannot open codec");
	cc->workaround_bugs = FF_BUG_AUTODETECT;
	m_codecContext = cc;
#endif

	switch (m_mediaType) {
	case AVMEDIA_TYPE_AUDIO:
		m_resampleContext = avresample_alloc_context();
		av_opt_set_int(m_resampleContext, "in_channel_layout", m_codecContext->channel_layout ? m_codecContext->channel_layout : av_get_default_channel_layout(m_codecContext->channels), 0);
		av_opt_set_int(m_resampleContext, "out_channel_layout", av_get_default_channel_layout(AUDIO_CHANNELS), 0);
		av_opt_set_int(m_resampleContext, "in_sample_rate", m_codecContext->sample_rate, 0);
		av_opt_set_int(m_resampleContext, "out_sample_rate", m_rate, 0);
		av_opt_set_int(m_resampleContext, "in_sample_fmt", m_codecContext->sample_fmt, 0);
		av_opt_set_int(m_resampleContext, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		avresample_open(m_resampleContext);
		if (!m_resampleContext) throw std::runtime_error("Cannot create resampling context");
		audioQueue.setSamplesPerSecond(AUDIO_CHANNELS * m_rate);
		break;
	case AVMEDIA_TYPE_VIDEO:
		// Setup software scaling context for YUV to RGB conversion
		width = m_codecContext->width;
		height = m_codecContext->height;
		m_swsContext = sws_getContext(
		  m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt,
		  width, height, AV_PIX_FMT_RGB24,
		  SWS_POINT, nullptr, nullptr, nullptr);
		break;
	default:  // Should never be reached but avoids compile warnings
		abort();
	}
}

void FFmpeg::operator()() {
	try { open(); } catch (std::exception const& e) { std::clog << "ffmpeg/error: Failed to open " << m_filename << ": " << e.what() << std::endl; m_quit = true; return; }
	m_duration = m_formatContext->duration / double(AV_TIME_BASE);
	audioQueue.setDuration(m_duration);
	int errors = 0;
	bool eof = false;
	while (!m_quit) {
		if(eof) {
			std::this_thread::sleep_for(100ms);
			continue;
		}
		try {
			if (audioQueue.wantSeek()) m_seekTarget = 0.0;
			if (m_seekTarget == m_seekTarget) seek_internal();
			decodePacket();
			errors = 0;
		} catch (eof_error&) {
			videoQueue.push(new Bitmap()); // EOF marker
			eof = true;
			std::clog << "ffmpeg/debug: done loading " << m_filename << std::endl;
		} catch (std::exception& e) {
			std::clog << "ffmpeg/error: " << m_filename << ": " << e.what() << std::endl;
			if (++errors > 2) { std::clog << "ffmpeg/error: FFMPEG terminating due to multiple errors" << std::endl; m_quit = true; }
		}
	}
	audioQueue.reset();
	videoQueue.reset();
	// TODO: use RAII for freeing resources (to prevent memory leaks)
	boost::mutex::scoped_lock l(s_avcodec_mutex); // avcodec_close is not thread-safe
	if (m_resampleContext) avresample_close(m_resampleContext);
	if (m_codecContext) avcodec_close(m_codecContext);
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(53, 17, 0)
	if (m_formatContext) avformat_close_input(&m_formatContext);
#else
	if (m_formatContext) av_close_input_file(m_formatContext);
#endif
}

void FFmpeg::seek(double time, bool wait) {
	m_seekTarget = time;
	videoQueue.reset(); audioQueue.reset(); // Empty these to unblock the internals in case buffers were full
	if (wait) while (!m_quit && m_seekTarget == m_seekTarget) std::this_thread::sleep_for(5ms);
}

void FFmpeg::seek_internal() {
	videoQueue.reset();
	audioQueue.reset();
	int flags = 0;
	if (m_seekTarget < m_position) flags |= AVSEEK_FLAG_BACKWARD;
	av_seek_frame(m_formatContext, -1, m_seekTarget * AV_TIME_BASE, flags);
	m_seekTarget = getNaN(); // Signal that seeking is done
}

class FfmpegError: public std::runtime_error {
public:
	FfmpegError(int errorValue): runtime_error(msgFmt(errorValue)) {}
private:
	static std::string msgFmt(int errorValue) {
		char message[AV_ERROR_MAX_STRING_SIZE];
		av_strerror(errorValue, message, AV_ERROR_MAX_STRING_SIZE);
		std::ostringstream oss;
		oss << "FfmpegError: code=" << errorValue << ", error=" << message;
		return oss.str();
	}
};

void FFmpeg::decodePacket() {
#if LIBAVCODEC_VERSION_INT >= (AV_VERSION_INT(57, 37, 0))
	AVPacket pkt;
	while (true) {
		// FIXME: we might want to take a look at m_quit.
		int ret = av_read_frame(m_formatContext, &pkt);
		if(ret == AVERROR_EOF) {
			// End of file: no more data to read.
			throw FFmpeg::eof_error();
		} else if(ret < 0) {
			throw FfmpegError(ret);
		}
		if (m_quit || m_seekTarget == m_seekTarget) return; // something weird required
		if (pkt.stream_index != m_streamId) return; // wrong stream
		ret = avcodec_send_packet(m_codecContext, &pkt);
		if(ret == AVERROR_EOF) {
			// End of file: no more data to read.
			throw FFmpeg::eof_error();
		} else if(ret == AVERROR(EAGAIN)) {
			// not enough data for decoder, read more
			continue;
		} else if(ret < 0) {
			throw FfmpegError(ret);
		}
		while (ret >= 0) {
			std::shared_ptr<AVFrame> frame(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
			ret = avcodec_receive_frame(m_codecContext, frame.get());
			if(ret == AVERROR_EOF) {
				// End of file: no more data.
				throw FFmpeg::eof_error();
			} else if(ret == AVERROR(EAGAIN)) {
				// not enough data to decode a frame, go read more and feed more to the decoder
				break;
			} else if(ret < 0) {
				throw FfmpegError(ret);
			}
			// frame is available here
			if (frame->pts != int64_t(AV_NOPTS_VALUE)) {
				m_position = double(frame->pts) * av_q2d(m_formatContext->streams[m_streamId]->time_base);
				if (m_formatContext->start_time != int64_t(AV_NOPTS_VALUE))
					m_position -= double(m_formatContext->start_time) / AV_TIME_BASE;
			}
			if (m_mediaType == AVMEDIA_TYPE_VIDEO) processVideo(frame.get()); else processAudio(frame.get());
			//av_frame_free(&frame);
		}
		av_packet_unref(&pkt);
	}
	return;
#else
	struct ReadFramePacket: public AVPacket {
		AVFormatContext* m_s;
		ReadFramePacket(AVFormatContext* s): m_s(s) {
			if (av_read_frame(s, this) < 0) throw FFmpeg::eof_error();
		}
#if LIBAVCODEC_VERSION_INT > (AV_VERSION_INT(55, 0, 0))
		~ReadFramePacket() { av_packet_unref(this); } //YES THEY DID IT AGAIN
#else
		~ReadFramePacket() { av_free_packet(this); }
#endif
	};

	// Read an AVPacket and decode it into AVFrames
	ReadFramePacket packet(m_formatContext);
	int packetSize = packet.size;
	while (packetSize) {
		if (packetSize < 0) throw std::logic_error("negative packet size?!");
		if (m_quit || m_seekTarget == m_seekTarget) return;
		if (packet.stream_index != m_streamId) return;
#if (LIBAVCODEC_VERSION_INT) < (AV_VERSION_INT(55,0,0))
		std::shared_ptr<AVFrame> frame(avcodec_alloc_frame(), &av_free);
#else
		std::shared_ptr<AVFrame> frame(av_frame_alloc(), [](AVFrame* ptr) { av_frame_free(&ptr); });
#endif
		int frameFinished = 0;
		int decodeSize = (m_mediaType == AVMEDIA_TYPE_VIDEO ?
		  avcodec_decode_video2(m_codecContext, frame.get(), &frameFinished, &packet) :
		  avcodec_decode_audio4(m_codecContext, frame.get(), &frameFinished, &packet));
		if (decodeSize < 0) return; // Packet didn't produce any output (could be waiting for B frames or something)
		packetSize -= decodeSize; // Move forward within the packet
		if (!frameFinished) continue;
		// Update current position if timecode is available
		if (frame->pkt_pts != int64_t(AV_NOPTS_VALUE)) {
			m_position = double(frame->pkt_pts) * av_q2d(m_formatContext->streams[m_streamId]->time_base);
			if (m_formatContext->start_time != int64_t(AV_NOPTS_VALUE))
				m_position -= double(m_formatContext->start_time) / AV_TIME_BASE;
		}
		if (m_mediaType == AVMEDIA_TYPE_VIDEO) processVideo(frame.get()); else processAudio(frame.get());
	}
#endif
}

void FFmpeg::processVideo(AVFrame* frame) {
	// Convert into RGB and scale the data
	int w = (m_codecContext->width+15)&~15;
	int h = m_codecContext->height;
	auto bitmap = new Bitmap();
	bitmap->timestamp = m_position;
	bitmap->fmt = pix::RGB;
	bitmap->resize(w, h);
	{
		uint8_t* data = bitmap->data();
		int linesize = w * 3;
		sws_scale(m_swsContext, frame->data, frame->linesize, 0, h, &data, &linesize);
	}
	videoQueue.push(bitmap);  // Takes ownership and may block until there is space
}

void FFmpeg::processAudio(AVFrame* frame) {
	// resample to output
	int16_t *output;
	int out_linesize;
	int out_samples = avresample_available(m_resampleContext) +
		av_rescale_rnd(avresample_get_delay(m_resampleContext) +
		frame->nb_samples, frame->sample_rate, m_rate, AV_ROUND_UP);
	av_samples_alloc((uint8_t**)&output, &out_linesize, AUDIO_CHANNELS, out_samples,
		AV_SAMPLE_FMT_S16, 0);
	out_samples = avresample_convert(m_resampleContext, (uint8_t**)&output, 0, out_samples,
		&frame->data[0], 0, frame->nb_samples);
	// The output is now an interleaved array of 16-bit samples
	std::vector<int16_t> m_output(output, output+out_samples*AUDIO_CHANNELS);
	audioQueue.push(m_output,m_position);
	av_freep(&output);
	m_position += double(out_samples)/m_rate;
}

