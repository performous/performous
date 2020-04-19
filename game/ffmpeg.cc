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
}

#define AUDIO_CHANNELS 2


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

AudioBuffer::uFvec AudioBuffer::makePreviewBuffer() {
        uFvec fvec(new_fvec(m_data.size() / 2));
        float previewVol = float(config["audio/preview_volume"].i()) / 100.0;
	{
		std::lock_guard<std::mutex> l(m_mutex);
		for (size_t rpos = 0, bpos = 0; rpos < m_data.size(); rpos += 2, bpos ++) {
			fvec->data[bpos] = (((da::conv_from_s16(m_data.at(rpos)) + da::conv_from_s16(m_data.at(rpos + 1))) / 2) / previewVol);
		}
	}
	return fvec;
};

bool AudioBuffer::wantMore() {
  return m_write_pos < m_read_pos + m_data.size() / 2;
}

/// Should the input stop waiting?
bool AudioBuffer::condition() {
    return m_quit || m_seek_asked || wantMore();
}

void AudioBuffer::operator()(const std::int16_t *data, size_t count, int64_t sample_position) {
       if (sample_position < 0) {
               std::clog << "ffmpeg/warning: Negative audio sample_position " << sample_position << " seconds, frame ignored." << std::endl;
               return;
       }
       
       std::unique_lock<std::mutex> l(m_mutex);
       if (sample_position < m_read_pos) {
           // frame to be dropped as being before read... arrived too last or due to a seek.
           return;
       }

	m_cond.wait(l, [this]{ return  condition(); });
	if (m_quit || m_seek_asked) return;
	
        if (m_write_pos != sample_position) {
            std::clog << "ffmpeg/debug: Gap in audio: expected=" << m_write_pos << " received=" << sample_position << '\n';
        }

        m_write_pos = sample_position;
        auto write_pos_in_ring = m_write_pos % m_data.size();
        auto first_hunk_size = std::min(count, m_data.size() - write_pos_in_ring);
	std::copy(data, data + first_hunk_size, m_data.begin() + write_pos_in_ring);
        // second part is when data wrapped in the ring buffer
	std::copy(data + first_hunk_size, data + count, m_data.begin());
	m_write_pos += count;
	m_cond.notify_all();
}

bool AudioBuffer::prepare(std::int64_t pos) {
	if (pos < 0) pos = 0;
	std::unique_lock<std::mutex> l(m_mutex, std::try_to_lock);
	if (!l.owns_lock()) return false;  // Didn't get lock, give up for now
	if (eof(pos)) return true;
	m_read_pos = pos;
	m_cond.notify_all();
	// Has enough been prebuffered already and is the requested position still within buffer
	return m_write_pos > m_read_pos + m_data.size() / 16 && m_write_pos <= m_read_pos + m_data.size();
}

// pos may be negative because upper layer may request 'extra time' before
// starting the play back. In this case, the buffer is filled of zero.
//
bool AudioBuffer::read(float* begin, size_t samples, std::int64_t pos, float volume) {
       if (eof(pos))
           return false;

        if (pos < 0) {
            size_t negative_samples;
            if (static_cast<std::int64_t>(samples) + pos > 0) negative_samples = samples - (samples + pos);
            else negative_samples = samples;

            // put zeros to negative positions
            std::fill(begin, begin + negative_samples, 0); 

            if (negative_samples == samples) return true;

            // if there are remaining samples to read in positive land, do the 'normal' read
            pos = 0;
            samples -= negative_samples;
        }

	std::unique_lock<std::mutex> l(m_mutex);
        if (static_cast<std::uint64_t>(pos) >= m_read_pos + m_data.size()
            || static_cast<std::uint64_t>(pos) < m_read_pos) {
            // in case request position is not in the current possible range, we trigger a seek
            // Note: m_write_pos is not checked on purpose: if pos is after
            // m_write_pos, zeros present in buffer will be returned
            std::fill(begin, begin + samples, 0); 
            m_read_pos = pos + samples;
            m_seek_asked = true;
            std::fill(m_data.begin(), m_data.end(), 0); 
            m_cond.notify_all();
            return true; 
        }

        for (size_t s = 0; s < samples; ++s) {
            begin[s] += volume * da::conv_from_s16(m_data[(m_read_pos + s) % m_data.size()]);
        }

	m_read_pos = pos + samples;
	m_cond.notify_all();
	return true;
}

AudioBuffer::AudioBuffer(fs::path const& file, unsigned int rate, size_t size):
          m_data(size), m_sps(rate * AUDIO_CHANNELS), ffmpeg(file, rate, std::ref(*this), nullptr), m_duration(ffmpeg.duration()) {
                  reader_thread = std::async(std::launch::async, [this] { 
            std::unique_lock<std::mutex> l(m_mutex);
            while (!m_quit) {
                 try {
                        if (m_seek_asked) {
                             auto seek_pos = m_read_pos / double(AV_TIME_BASE);
                             m_seek_asked = false;
                             m_write_pos = m_read_pos;
                             l.unlock();
                             ffmpeg.seek(seek_pos);
                        } else 
                             l.unlock();
                          
                        ffmpeg.handleOneFrame();

                        l.lock();
                 } catch (FFmpeg::eof_error&) {
                        l.lock();
                        // now we know exact eof_pos
                        m_eof_pos = m_write_pos;
                        // Wait here on eof: either quit is asked, either a new seek
                        // was asked and return back reading frames
                        m_cond.wait(l, [this]{ return m_quit || m_seek_asked; });
                 } catch (std::exception& e) {
                        l.lock();
                        m_eof_pos = m_write_pos;
                        m_cond.wait(l, [this]{ return m_quit || m_seek_asked; });
                 }
             }
    });
}

AudioBuffer::~AudioBuffer() {
    {
	std::unique_lock<std::mutex> l(m_mutex);
        m_read_pos = 0;
        m_write_pos = 0;
        m_data.clear();
        m_quit = true;
    }
    m_cond.notify_all();
    reader_thread.get();
}

static void printFFmpegInfo() {
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
};

FFmpeg::FFmpeg(fs::path const& _filename, unsigned int rate, AudioCb audiocb, VideoCb videocb = nullptr) :
  m_filename(_filename),
  m_rate(rate),
  handleAudioData(audiocb),
  handleVideoData(videocb) {
      static std::once_flag static_infos;
      std::call_once(static_infos, &printFFmpegInfo);

        auto mediaType = m_rate ? AVMEDIA_TYPE_AUDIO : AVMEDIA_TYPE_VIDEO;
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
	m_streamId = av_find_best_stream(m_formatContext.get(), mediaType, -1, -1, &codec, 0);
	if (m_streamId < 0) throw std::runtime_error("No suitable track found");

	decltype(m_codecContext) pCodecCtx{avcodec_alloc_context3(codec), avcodec_free_context};
	avcodec_parameters_to_context(pCodecCtx.get(), m_formatContext->streams[m_streamId]->codecpar);
	{
		static std::mutex s_avcodec_mutex; 
		// ffmpeg documentation is clear on the fact that avcodec_open2 is not thread safe.
		std::lock_guard<std::mutex> l(s_avcodec_mutex);
		if (avcodec_open2(pCodecCtx.get(), codec, nullptr) < 0) throw std::runtime_error("Cannot open codec");
	}
	pCodecCtx->workaround_bugs = FF_BUG_AUTODETECT;
	m_codecContext = std::move(pCodecCtx);

        m_processingFn = (mediaType == AVMEDIA_TYPE_AUDIO) ? &FFmpeg::processAudio : &FFmpeg::processVideo;

	switch (mediaType) {
	case AVMEDIA_TYPE_AUDIO:
		m_resampleContext.reset(swr_alloc());
		if (!m_resampleContext) throw std::runtime_error("Cannot create resampling context");
		av_opt_set_int(m_resampleContext.get(), "in_channel_layout", m_codecContext->channel_layout ? m_codecContext->channel_layout : av_get_default_channel_layout(m_codecContext->channels), 0);
		av_opt_set_int(m_resampleContext.get(), "out_channel_layout", av_get_default_channel_layout(AUDIO_CHANNELS), 0);
		av_opt_set_int(m_resampleContext.get(), "in_sample_rate", m_codecContext->sample_rate, 0);
		av_opt_set_int(m_resampleContext.get(), "out_sample_rate", m_rate, 0);
		av_opt_set_int(m_resampleContext.get(), "in_sample_fmt", m_codecContext->sample_fmt, 0);
		av_opt_set_int(m_resampleContext.get(), "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
		swr_init(m_resampleContext.get());
		break;
	case AVMEDIA_TYPE_VIDEO:
		// Setup software scaling context for YUV to RGB conversion
		m_swsContext.reset(sws_getContext(
		  m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt,
		  m_codecContext->width, m_codecContext->height, AV_PIX_FMT_RGB24,
		  SWS_POINT, nullptr, nullptr, nullptr));
		break;
	default:  // Should never be reached but avoids compile warnings
		abort();
	}
} 

double FFmpeg::duration() const { return m_formatContext->duration / double(AV_TIME_BASE); }

void FFmpeg::avformat_close_input(AVFormatContext *fctx) {
	if (fctx) ::avformat_close_input(&fctx);
}
void FFmpeg::avcodec_free_context(AVCodecContext *avctx) {
         if (avctx == nullptr) return;
         avcodec_close(avctx);
         ::avcodec_free_context(&avctx);
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

void FFmpeg::handleOneFrame() {
	bool read_one = false;
	do {
		Packet pkt;
		auto ret = av_read_frame(m_formatContext.get(), &pkt);
		if(ret == AVERROR_EOF) {
			// End of file: no more data to read.
			throw FFmpeg::eof_error();
		} else if(ret < 0) {
			throw FfmpegError(ret);
		}

		if (pkt.stream_index != m_streamId) continue;

		decodePacket(pkt);
		read_one = true;
	} while (!read_one);
}

void FFmpeg::seek(double time) {
	int flags = 0;
	if (time < m_position) flags |= AVSEEK_FLAG_BACKWARD;
	av_seek_frame(m_formatContext.get(), -1, time * AV_TIME_BASE, flags);
        m_position_in_48k_frames = -1; //kill previous position
}

void FFmpeg::decodePacket(Packet &pkt) {
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
	while (ret >= 0) {
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
                        m_position = new_position;
		}
		(this->*m_processingFn)(std::move(frame));
	}
}

void FFmpeg::processVideo(uFrame frame) {
	// Convert into RGB and scale the data
	int w = (m_codecContext->width + 15) & ~15;
	auto h = m_codecContext->height;
	Bitmap f;
	f.timestamp = m_position;
	f.fmt = pix::RGB;
	f.resize(w, h);
	{
		uint8_t* data = f.data();
		int linesize = w * 3;
		sws_scale(m_swsContext.get(), frame->data, frame->linesize, 0, h, &data, &linesize);
	}
	handleVideoData(std::move(f));  // Takes ownership and may block until there is space
}

void FFmpeg::processAudio(uFrame frame) {
	// resample to output
	int16_t *output;
	int out_samples = swr_get_out_samples(m_resampleContext.get(), frame->nb_samples);
	av_samples_alloc((uint8_t**)&output, nullptr, AUDIO_CHANNELS, out_samples,
		AV_SAMPLE_FMT_S16, 0);
	out_samples = swr_convert(m_resampleContext.get(), (uint8_t**)&output, out_samples,
		(const uint8_t**)&frame->data[0], frame->nb_samples);
	// The output is now an interleaved array of 16-bit samples
        if (m_position_in_48k_frames == -1) {
                m_position_in_48k_frames = m_position * m_rate + 0.5;
        }
	handleAudioData(output, out_samples * AUDIO_CHANNELS, m_position_in_48k_frames * AUDIO_CHANNELS /* pass in samples */);
	av_freep(&output);
        m_position_in_48k_frames += out_samples;
	m_position += frame->nb_samples * av_q2d(m_formatContext->streams[m_streamId]->time_base);
}

