#include "ffmpeg.hh"

#include "chrono.hh"
#include "config.hh"
#include "screen_songs.hh"
#include "util.hh"

#include "aubio/aubio.h"
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
#include SWRESAMPLE_INCLUDE
#include AVUTIL_INCLUDE
#include AVUTIL_OPT_INCLUDE
#include AVUTIL_MATH_INCLUDE
#include AVUTIL_ERROR_INCLUDE
#include </usr/include/ffmpeg/libavutil/timestamp.h>
}

#define AUDIO_CHANNELS 2

/*static*/ std::mutex FFmpeg::s_avcodec_mutex;

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

bool VideoFifo::tryPop(Bitmap& f) {
	std::unique_lock<std::mutex> l(m_mutex);
	if (m_queue.empty()) return false; // Nothing to deliver
	if (m_queue.front().buf.empty()) { m_eof = true; return false; }
	f = std::move(m_queue.front());
	m_queue.pop_front();
	m_cond.notify_all();
	m_timestamp = f.timestamp;
	return true;
}

void VideoFifo::push(Bitmap&& f) {
	std::unique_lock<std::mutex> l(m_mutex);
	m_cond.wait(l, [this]{ return m_queue.size() < m_max; });
	if (m_queue.empty()) m_timestamp = f.timestamp;
	m_queue.emplace_back(std::move(f));
}

void VideoFifo::reset() {
	std::unique_lock<std::mutex> l(m_mutex);
	m_queue.clear();
	m_cond.notify_all();
	m_eof = false;
}

void AudioBuffer::reset() {
	{
		std::unique_lock<mutex> l(m_mutex);
		m_data.clear();
		m_pos = 0;
	}
	m_cond.notify_one();
}

void AudioBuffer::quit() {
	m_quit.store(true);
	m_cond.notify_one();
}

AudioBuffer::uFvec AudioBuffer::makePreviewBuffer() {
        uFvec fvec;
	{
		std::unique_lock<mutex> l(m_mutex);
                m_cond.wait(l, [this]{ return !m_data.empty(); });
		fvec.reset(new_fvec(m_data.size() / 2));
		float previewVol = float(config["audio/preview_volume"].i()) / 100.0;
		for (size_t rpos = 0, bpos = 0; rpos < m_data.size(); rpos += 2, bpos ++) {
			fvec->data[bpos] = (((da::conv_from_s16(m_data[rpos]) + da::conv_from_s16(m_data[rpos + 1])) / 2) / previewVol);
		}
	}
	m_cond.notify_one();
	return fvec;
};

void AudioBuffer::push(std::vector<std::int16_t> const& data, double timestamp) {
	std::unique_lock<mutex> l(m_mutex);
	m_cond.wait(l, [this]{ return condition(); });
	if (m_quit) return;
	if (timestamp < 0.0) {
		std::clog << "ffmpeg/warning: Negative audio timestamp " << timestamp << " seconds, frame ignored." << std::endl;
		return;
	}
	// Insert silence at the beginning if the stream starts later than 0.0
	if (m_pos == 0 && timestamp >= 0.0) {
		m_pos = std::round(float(timestamp * getSamplesPerSecond()));
		if (m_pos % 2 == 1) { m_pos++; timestamp = float(m_pos) / getSamplesPerSecond(); }
		m_data.resize(m_pos, 0);
	}
	m_data.insert(m_data.end(), data.begin(), data.end());
	m_pos += data.size();
	m_cond.notify_one();
}

bool AudioBuffer::prepare(std::int64_t pos) {
	std::unique_lock<mutex> l(m_mutex, std::try_to_lock);
	if (!l.owns_lock()) return false;  // Didn't get lock, give up for now
	if (eof(pos)) return true;
	if (pos < 0) pos = 0;
	m_posReq = pos;
	wakeups();
	// Has enough been prebuffered already and is the requested position still within buffer
	return m_pos > m_posReq + m_data.capacity() / 16 && m_pos <= m_posReq + m_data.size();
}

bool AudioBuffer::operator()(float* begin, size_t samples, std::int64_t pos, float volume) {
	std::unique_lock<mutex> l(m_mutex);
	size_t idx = pos + m_data.size() - m_pos;
	for (size_t s = 0; s < samples; ++s, ++idx) {
		if (idx < m_data.size()) begin[s] += volume * da::conv_from_s16(m_data[idx]);
	}
	m_posReq = std::max<std::int64_t>(0, pos + samples);
	wakeups();
	return !eof(pos);
}

FFmpeg::FFmpeg(fs::path const& _filename, unsigned int rate):
  m_filename(_filename), m_rate(rate),
  m_mediaType(rate ? AVMEDIA_TYPE_AUDIO : AVMEDIA_TYPE_VIDEO),
  m_thread(std::async(std::launch::async, std::ref(*this)))
{
    static std::once_flag static_infos;
    std::call_once(static_infos, [] {
		bool matches = LIBAVUTIL_VERSION_INT == avutil_version() &&
                                LIBAVCODEC_VERSION_INT == avcodec_version() &&
                                LIBAVFORMAT_VERSION_INT == avformat_version() &&
                                LIBSWSCALE_VERSION_INT == swscale_version();
		if (matches) {
			std::clog << "ffmpeg/info: "
			  " avutil:" + ffversion(LIBAVUTIL_VERSION_INT) +
			  " avcodec:" + ffversion(LIBAVCODEC_VERSION_INT) +
			  " avformat:" + ffversion(LIBAVFORMAT_VERSION_INT) +
			  " swresample:" + ffversion(LIBSWRESAMPLE_VERSION_INT) +
			  " swscale:" + ffversion(LIBSWSCALE_VERSION_INT)
			  << std::endl;
		} else {
			std::clog << "ffmpeg/error: header/lib version mismatch:"
			  " avutil:" + ffversion(LIBAVUTIL_VERSION_INT) + "/" + ffversion(avutil_version()) +
			  " avcodec:" + ffversion(LIBAVCODEC_VERSION_INT) + "/" + ffversion(avcodec_version()) +
			  " avformat:" + ffversion(LIBAVFORMAT_VERSION_INT) + "/" + ffversion(avformat_version()) +
			  " swresample:" + ffversion(LIBSWRESAMPLE_VERSION_INT) + "/" + ffversion(swresample_version()) +
			  " swscale:" + ffversion(LIBSWSCALE_VERSION_INT) + "/" + ffversion(swscale_version())
			  << std::endl;
		}
#if (LIBAVFORMAT_VERSION_INT) < (AV_VERSION_INT(58,0,0))
                av_register_all();
#endif
	});
}

FFmpeg::~FFmpeg() {
	m_quit.set_value();
	videoQueue.reset();
	audioQueue.quit();
	m_thread.get();
        {
            std::lock_guard<std::mutex> l(s_avcodec_mutex); // avcodec_close is not thread-safe
            if (m_codecContext) avcodec_close(m_codecContext.get());
        }
}

void FFmpeg::avformat_close_input(AVFormatContext *fctx) {
	if (fctx) ::avformat_close_input(&fctx);
}
void FFmpeg::avcodec_free_context(AVCodecContext *avctx) {
	::avcodec_free_context(&avctx);
}

void FFmpeg::open() {
	av_log_set_level(AV_LOG_ERROR);
	{
		AVFormatContext *avfctx = nullptr;
		if (avformat_open_input(&avfctx, m_filename.string().c_str(), nullptr, nullptr)) throw std::runtime_error("Cannot open input file");
		m_formatContext.reset(avfctx);
	}
	if (avformat_find_stream_info(m_formatContext.get(), nullptr) < 0) throw std::runtime_error("Cannot find stream information");
	m_formatContext->flags |= AVFMT_FLAG_GENPTS;
	// Find a track and open the codec
	AVCodec* codec = nullptr;
	m_streamId = av_find_best_stream(m_formatContext.get(), (AVMediaType)m_mediaType, -1, -1, &codec, 0);
	if (m_streamId < 0) throw std::runtime_error("No suitable track found");

	decltype(m_codecContext) pCodecCtx{avcodec_alloc_context3(codec), avcodec_free_context};
	avcodec_parameters_to_context(pCodecCtx.get(), m_formatContext->streams[m_streamId]->codecpar);
	{
		// ffmpeg documentation is clear on the fact that avcodec_open2 is not thread safe.
		std::lock_guard<std::mutex> l(s_avcodec_mutex);
		if (avcodec_open2(pCodecCtx.get(), codec, nullptr) < 0) throw std::runtime_error("Cannot open codec");
	}
	pCodecCtx->workaround_bugs = FF_BUG_AUTODETECT;
	m_codecContext = std::move(pCodecCtx);

	switch (m_mediaType) {
	case AVMEDIA_TYPE_AUDIO:
		m_resampleContext.reset(swr_alloc());
		av_opt_set_int(m_resampleContext.get(), "in_channel_layout", m_codecContext->channel_layout ? m_codecContext->channel_layout : av_get_default_channel_layout(m_codecContext->channels), 0);
		av_opt_set_int(m_resampleContext.get(), "out_channel_layout", av_get_default_channel_layout(AUDIO_CHANNELS), 0);
		av_opt_set_int(m_resampleContext.get(), "in_sample_rate", m_codecContext->sample_rate, 0);
		av_opt_set_int(m_resampleContext.get(), "out_sample_rate", m_rate, 0);
		av_opt_set_int(m_resampleContext.get(), "in_sample_fmt", m_codecContext->sample_fmt, 0);
		av_opt_set_int(m_resampleContext.get(), "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		swr_init(m_resampleContext.get());
		if (!m_resampleContext) throw std::runtime_error("Cannot create resampling context");
		audioQueue.setSamplesPerSecond(AUDIO_CHANNELS * m_rate);
		break;
	case AVMEDIA_TYPE_VIDEO:
		// Setup software scaling context for YUV to RGB conversion
		width = m_codecContext->width;
		height = m_codecContext->height;
		m_swsContext.reset(sws_getContext(
		  m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt,
		  width, height, AV_PIX_FMT_RGB24,
		  SWS_POINT, nullptr, nullptr, nullptr));
		break;
	default:  // Should never be reached but avoids compile warnings
		abort();
	}
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

struct FFmpeg::Packet: public AVPacket {
	Packet() {
		av_init_packet(this);
		this->data = nullptr;
		this->size = 0;
	}

	~Packet() { av_packet_unref(this); }
};

void FFmpeg::operator()() {
	try { open(); } catch (std::exception const& e) { std::clog << "ffmpeg/error: Failed to open " << m_filename << ": " << e.what() << std::endl; return; }
	m_duration = m_formatContext->duration / double(AV_TIME_BASE);
	audioQueue.setDuration(m_duration);
	int errors = 0;
	bool eof = false;
	std::clog << "audio/debug: FFmpeg processing " << m_filename.filename().string() << std::endl;
	Packet pkt;
	while (!terminating() && !eof) {
		try {
			auto ret = av_read_frame(m_formatContext.get(), &pkt);
			if(ret == AVERROR_EOF) {
				// End of file: no more data to read.
				throw FFmpeg::eof_error();
			} else if(ret < 0) {
				throw FfmpegError(ret);
			}

			if (pkt.stream_index == m_streamId) {

				if (audioQueue.wantSeek()) m_seekTarget = 0.0;
				if (m_seekTarget == m_seekTarget) seek_internal();
				decodePacket(pkt);
				errors = 0;
			}
		} catch (eof_error&) {
			videoQueue.push(Bitmap()); // EOF marker
			eof = true;
			std::clog << "ffmpeg/debug: done loading " << m_filename << std::endl;
		} catch (std::exception& e) {
			std::clog << "ffmpeg/error: " << m_filename << ": " << e.what() << std::endl;
			//if (++errors > 2) { std::clog << "ffmpeg/error: FFMPEG terminating due to multiple errors" << std::endl; break; }
		}
		av_packet_unref(&pkt);
	}
}

void FFmpeg::seek(double time, bool wait) {
	m_seekTarget = time;
	videoQueue.reset(); audioQueue.reset(); // Empty these to unblock the internals in case buffers were full
	if (wait) while (!terminating() && m_seekTarget == m_seekTarget) std::this_thread::sleep_for(5ms);
}

void FFmpeg::seek_internal() {
	videoQueue.reset();
	audioQueue.reset();
	int flags = 0;
	if (m_seekTarget < m_position) flags |= AVSEEK_FLAG_BACKWARD;
	av_seek_frame(m_formatContext.get(), -1, m_seekTarget * AV_TIME_BASE, flags);
	m_seekTarget = getNaN(); // Signal that seeking is done
}

void FFmpeg::decodePacket(Packet &pkt) {

    if (pkt.stream_index > 1) {
        AVRational *time_base = &m_formatContext->streams[pkt.stream_index]->time_base;
        char p1[AV_TS_MAX_STRING_SIZE];
        char p2[AV_TS_MAX_STRING_SIZE];
        char p3[AV_TS_MAX_STRING_SIZE];
        char p4[AV_TS_MAX_STRING_SIZE];
        char p5[AV_TS_MAX_STRING_SIZE];
        char p6[AV_TS_MAX_STRING_SIZE];
        printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d % s\n",
                av_ts_make_string(p1, pkt.pts), av_ts_make_time_string(p6, pkt.pts, time_base),
                av_ts_make_string(p2, pkt.dts), av_ts_make_time_string(p4, pkt.dts, time_base),
                av_ts_make_string(p3, pkt.duration), av_ts_make_time_string(p5, pkt.duration, time_base),
                pkt.stream_index, m_filename.c_str());
    }
	auto ret = avcodec_send_packet(m_codecContext.get(), &pkt);
	if(ret == AVERROR_EOF) {
		// End of file: no more data to read.
		throw FFmpeg::eof_error();
	} else if(ret == AVERROR(EAGAIN)) {
		// no room for new data, need to get more frames out of the decoder by
		// calling avcodec_receive_frame()
	} else if(ret < 0) {
		throw FfmpegError(ret);
	}
	while (!terminating() && ret >= 0) {
		uFrame frame{av_frame_alloc()};
		ret = avcodec_receive_frame(m_codecContext.get(), frame.get());
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
			auto new_position = double(frame->pts) * av_q2d(m_formatContext->streams[m_streamId]->time_base);
			if (m_formatContext->streams[m_streamId]->start_time != int64_t(AV_NOPTS_VALUE))
				new_position -= double(m_formatContext->streams[m_streamId]->start_time) * av_q2d(m_formatContext->streams[m_streamId]->time_base);
			//if (m_formatContext->streams[m_streamId]->first_pts != int64_t(AV_NOPTS_VALUE))
			//	m_position -= double(m_formatContext->streams[m_streamId]->first_pts) / AV_TIME_BASE;
                        if (new_position > 0)
                            m_position = new_position;

                        
		}
		if (m_mediaType == AVMEDIA_TYPE_VIDEO) processVideo(std::move(frame)); else processAudio(std::move(frame));
	}
}

void FFmpeg::processVideo(uFrame frame) {
	// Convert into RGB and scale the data
	int w = (m_codecContext->width+15)&~15;
	int h = m_codecContext->height;
	Bitmap f;
	f.timestamp = m_position;
	f.fmt = pix::RGB;
	f.resize(w, h);
	{
		uint8_t* data = f.data();
		int linesize = w * 3;
		sws_scale(m_swsContext.get(), frame->data, frame->linesize, 0, h, &data, &linesize);
	}
	videoQueue.push(std::move(f));  // Takes ownership and may block until there is space
}

void FFmpeg::processAudio(uFrame frame) {
	// resample to output
	int16_t *output;
	int out_linesize;
	int out_samples = swr_get_out_samples(m_resampleContext.get(), frame->nb_samples);
	av_samples_alloc((uint8_t**)&output, &out_linesize, AUDIO_CHANNELS, out_samples,
		AV_SAMPLE_FMT_S16, 0);
	out_samples = swr_convert(m_resampleContext.get(), (uint8_t**)&output, out_samples,
		(const uint8_t**)&frame->data[0], frame->nb_samples);
	// The output is now an interleaved array of 16-bit samples
	std::vector<int16_t> m_output(output, output+out_samples*AUDIO_CHANNELS);
	audioQueue.push(m_output, m_position);
	av_freep(&output);
	m_position += double(out_samples)/m_rate;
}

