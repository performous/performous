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
#include <algorithm>

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/error.h>
#include <libavutil/replaygain.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
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
    uFvec fvec(new_fvec(static_cast<uint_t>(m_data.size() / 2)));
    float previewVol = float(config["audio/preview_volume"].ui()) / 100.0f;
    {
        std::lock_guard<std::mutex> l(m_mutex);
        for (size_t rpos = 0, bpos = 0; rpos < m_data.size(); rpos += 2, bpos ++) {
            fvec->data[bpos] = (((da::conv_from_s16(m_data.at(rpos)) + da::conv_from_s16(m_data.at(rpos + 1))) / 2) / previewVol);
        }
    }
    return fvec;
}

bool AudioBuffer::wantMore() {
    return m_write_pos < m_read_pos + static_cast<std::int64_t>(m_data.size() / 2);
}

/// Should the input stop waiting?
bool AudioBuffer::condition() {
    return m_quit || m_seek_asked || wantMore();
}

void AudioBuffer::operator()(const std::int16_t *data, std::int64_t count, std::int64_t sample_position) {
    if (sample_position < 0) {
        std::clog << "ffmpeg/warning: Negative audio sample_position " << sample_position << " seconds, frame ignored." << std::endl;
        return;
    }

    std::unique_lock<std::mutex> l(m_mutex);
    if (sample_position < m_read_pos) {
        // frame to be dropped as being before read... arrived too last or due to a seek.
        std::clog << "ffmpeg/debug: Frame dropped sample_position [" << sample_position << "] seconds, < read_position [" << m_read_pos << "]" << std::endl;
        return;
    }

    std::int64_t amount = std::min( count, 32L );
    std::stringstream msg;
    msg << "Audio Data is [";
    for ( ssize_t i=0; i<amount; i++)
    {
        msg << data[i];
        if ( i <amount-1 )
            msg << ", ";
    }
    msg << "] ...";
    std::clog << "ffmpeg/debug: " << msg.str() << "]" << std::endl;

    m_cond.wait(l, [this]{ return  condition(); });
    if (m_quit || m_seek_asked)
    {
        std::clog << "ffmpeg/debug: Frame Dropped, Returning for a <quit> or <seek> (ignored data)" << std::endl;
        return;
    }

    if (m_write_pos != sample_position) {
        std::clog << "ffmpeg/debug: Gap in audio: expected=" << m_write_pos << " received=" << sample_position << '\n';
    }

    m_write_pos = sample_position;
    std::int64_t write_pos_in_ring = m_write_pos % static_cast<std::int64_t>(m_data.size());
    auto first_hunk_size = std::min(count, static_cast<std::int64_t>(m_data.size()) - write_pos_in_ring);
    std::copy(data, data + first_hunk_size, m_data.begin() + write_pos_in_ring);
    // second part is when data wrapped in the ring buffer
    std::copy(data + first_hunk_size, data + count, m_data.begin());
    m_write_pos += count;
    m_cond.notify_all();
}

bool AudioBuffer::prepare(std::int64_t pos) {
    // perform fake read to trigger any potential seek
    if (!read(nullptr, 0, pos, 1)) return true;

    std::unique_lock<std::mutex> l(m_mutex);
    // Has enough been prebuffered already and is the requested position still within buffer
    auto ring_size = static_cast<std::int64_t>(m_data.size());
    return m_write_pos > m_read_pos + ring_size / 16 && m_write_pos <= m_read_pos + ring_size;
}

// pos may be negative because upper layer may request 'extra time' before
// starting the play back. In this case, the buffer is filled of zero.
//
bool AudioBuffer::read(float* begin, std::int64_t samples, std::int64_t pos, float volume) {
    if (pos < 0) {
        std::int64_t negative_samples;
        if (samples + pos > 0) negative_samples = samples - (samples + pos);
        else negative_samples = samples;

        // put zeros to negative positions
        std::fill(begin, begin + negative_samples, 0);

        if (negative_samples == samples) return true;

        // if there are remaining samples to read in positive land, do the 'normal' read
        pos = 0;
        samples -= negative_samples;
    }

    std::unique_lock<std::mutex> l(m_mutex);
    if (eof(pos + samples) || m_quit)
        return false;

    // one cannot read more data than the size of buffer
    std::int64_t size = static_cast<std::int64_t>(m_data.size());
    samples = std::min(samples, size);
    if (pos >= m_read_pos + size - samples || pos < m_read_pos) {
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

    for (std::int64_t s = 0; s < samples; ++s) {
        begin[s] += volume * da::conv_from_s16(m_data[static_cast<size_t>((m_read_pos + s)) % m_data.size()]);
    }

    m_read_pos = pos + samples;
    m_cond.notify_all();
    return true;
}

double AudioBuffer::duration() { return m_duration; }
double AudioBuffer::replayGain() { return m_replaygain; }   // perceived loudness in dB

AudioBuffer::AudioBuffer(fs::path const& file, unsigned rate, size_t size):
    m_data(size), m_sps(rate * AUDIO_CHANNELS) {
        auto ffmpeg = std::make_unique<AudioFFmpeg>(file, rate, std::ref(*this));
        const_cast<double&>(m_duration) = ffmpeg->duration();
        const_cast<double&>(m_replaygain) = ffmpeg->replayGain();
        reader_thread = std::async(std::launch::async, [this, ffmpeg = std::move(ffmpeg)] {
            auto errors = 0u;
            std::unique_lock<std::mutex> l(m_mutex);
            while (!m_quit) {
                if (m_seek_asked) {
                    m_seek_asked = false;
                    m_write_pos = m_read_pos;
                    auto seek_pos = static_cast<double>(m_read_pos) / double(AV_TIME_BASE);

                    UnlockGuard<decltype(l)> unlocked(l); // release lock during seek
                    ffmpeg->seek(seek_pos);
                    continue;
                }

                try {
                    UnlockGuard<decltype(l)> unlocked(l); // release lock during possibly blocking ffmpeg stuff

                    ffmpeg->handleOneFrame();
                    errors = 0;
                } catch (const FFmpeg::Eof&) {
                    // now we know exact eof_pos
                    m_eof_pos = m_write_pos;
                    // Wait here on eof: either quit is asked, either a new seek
                    // was asked and return back reading frames
                    m_cond.wait(l, [this]{ return m_quit || m_seek_asked; });
                } catch (const std::exception& e) {
                    UnlockGuard<decltype(l)> unlocked(l); // unlock while doing IOs
                    std::clog << "ffmpeg/error: " << e.what() << std::endl;
                    if (++errors > 2) std::clog << "ffmpeg/error: FFMPEG terminating due to multiple errors" << std::endl;
                }
            }
        });
}

AudioBuffer::~AudioBuffer() {
    {
        std::unique_lock<std::mutex> l(m_mutex);
        m_read_pos = 0;
        m_write_pos = 0;
        std::fill(m_data.begin(), m_data.end(), 0);
        m_quit = true;
    }
    m_cond.notify_all();
    reader_thread.get();
}

static void printFFmpegInfo() {
    bool matches = LIBAVUTIL_VERSION_INT == avutil_version() &&
        LIBAVCODEC_VERSION_INT == avcodec_version() &&
        LIBAVFORMAT_VERSION_INT == avformat_version() &&
        LIBAVFILTER_VERSION_INT == avfilter_version() &&
        LIBSWSCALE_VERSION_INT == swscale_version();

    if (matches) {
        std::clog << "ffmpeg/info: "
            " avutil:" + ffversion(LIBAVUTIL_VERSION_INT) +
            " avcodec:" + ffversion(LIBAVCODEC_VERSION_INT) +
            " avformat:" + ffversion(LIBAVFORMAT_VERSION_INT) +
            " avfilter:" + ffversion(LIBAVFILTER_VERSION_INT) +
            " swresample:" + ffversion(LIBSWRESAMPLE_VERSION_INT) +
            " swscale:" + ffversion(LIBSWSCALE_VERSION_INT)
            << std::endl;
    } else {
        std::clog << "ffmpeg/error: header/lib version mismatch:"
            " avutil:" + ffversion(LIBAVUTIL_VERSION_INT) + "/" + ffversion(avutil_version()) +
            " avcodec:" + ffversion(LIBAVCODEC_VERSION_INT) + "/" + ffversion(avcodec_version()) +
            " avformat:" + ffversion(LIBAVFORMAT_VERSION_INT) + "/" + ffversion(avformat_version()) +
            " avfilter:" + ffversion(LIBAVFILTER_VERSION_INT) + "/" + ffversion(avfilter_version()) +
            " swresample:" + ffversion(LIBSWRESAMPLE_VERSION_INT) + "/" + ffversion(swresample_version()) +
            " swscale:" + ffversion(LIBSWSCALE_VERSION_INT) + "/" + ffversion(swscale_version())
            << std::endl;
    }
#if (LIBAVFORMAT_VERSION_INT) < (AV_VERSION_INT(58,0,0))
    av_register_all();
#endif
}

class FFmpeg::Error: public std::runtime_error {
  public:
    Error(const FFmpeg &self, int errorValue): std::runtime_error(msgFmt(self, errorValue)) {}
  private:
    static std::string msgFmt(const FFmpeg &self, int errorValue) {
        char message[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(errorValue, message, AV_ERROR_MAX_STRING_SIZE);
        std::ostringstream oss;
        oss << "FFmpeg Error: Processing file " << self.m_filename << " code=" << errorValue << ", error=" << message;
        return oss.str();
    }
};

FFmpeg::FFmpeg(fs::path const& _filename, int mediaType) : m_filename(_filename) {
    static std::once_flag static_infos;
    std::call_once(static_infos, &printFFmpegInfo);

    av_log_set_level(AV_LOG_ERROR);
    {
        AVFormatContext *avfctx = nullptr;
        auto err = avformat_open_input(&avfctx, m_filename.string().c_str(), nullptr, nullptr);
        if (err) throw Error(*this, err);
        m_formatContext.reset(avfctx);
    }
    auto err = avformat_find_stream_info(m_formatContext.get(), nullptr);
    if (err < 0) throw Error(*this, err);
    m_formatContext->flags |= AVFMT_FLAG_GENPTS;
    // Find a track and open the codec
#if (LIBAVFORMAT_VERSION_INT) >= (AV_VERSION_INT(59, 0, 100))
    const
#endif
    AVCodec* codec = nullptr;
    m_streamId = av_find_best_stream(m_formatContext.get(), static_cast<AVMediaType>(mediaType), -1, -1, &codec, 0);
    if (m_streamId < 0) throw Error(*this, m_streamId);

    // Possibly the stream defines a Replay Gain factor; read it
    std::clog << "ffmpeg/debug: FFmpeg::FFmpeg Replay Gain on [" << _filename << "] reading..." << std::endl;
    readReplayGain(m_formatContext->streams[m_streamId]);

    decltype(m_codecContext) pCodecCtx{avcodec_alloc_context3(codec), avcodec_free_context};
    avcodec_parameters_to_context(pCodecCtx.get(), m_formatContext->streams[m_streamId]->codecpar);
    {
        static std::mutex s_avcodec_mutex;
        // ffmpeg documentation is clear on the fact that avcodec_open2 is not thread safe.
        std::lock_guard<std::mutex> l(s_avcodec_mutex);
        err = avcodec_open2(pCodecCtx.get(), codec, nullptr);
        if (err < 0) throw Error(*this, err);
    }
    pCodecCtx->workaround_bugs = FF_BUG_AUTODETECT;
    m_codecContext = std::move(pCodecCtx);
}


/**
  * \brief    Fetch the Replay Gain "loudness" factor from the stream
  *
  * \details  Replay Gain is a decibels "loudness" factor that can be used to normalise
  *           the human-perceived loudness of a given audio sample.  It is useful for
  *           normalising the volume of recordings mastered at different volumes.
  *           Libav reads this information in, storing it in the side channel blocks.
  *           This function requests this value from the side channel.
  *
  *           Ref: https://en.wikipedia.org/wiki/ReplayGain
  *                https://www.smithsonianmag.com/smart-news/music-does-get-louder-every-year-835681/
  *
  * \returns  True if a Replay Gain value was found in the stream
  */
bool FFmpeg::readReplayGain(const AVStream *stream)
{
    bool rg_read = false;
    // assert(stream)
    m_replaygain = 0.0;
    if (stream != nullptr) {
        size_t replay_gain_size = 0;
        const AVReplayGain *replay_gain = (AVReplayGain *)av_stream_get_side_data(stream, AV_PKT_DATA_REPLAYGAIN, &replay_gain_size);
        if (replay_gain_size > 0 && replay_gain != nullptr) {
            m_replaygain = static_cast<double>(replay_gain->track_gain);
            m_replaygain /= 100000.0;   // convert from microbels to decibels
            rg_read = true;
            std::clog << "ffmpeg/debug: readReplayGain() - GAIN IS [" << std::fixed << std::setprecision(2) << m_replaygain << "] dB" << std::endl;
        }
        else {
            std::clog << "ffmpeg/debug: readReplayGain() - no gain" << std::endl;
        }
    }
    return rg_read;
}


VideoFFmpeg::VideoFFmpeg(fs::path const& filename, VideoCb videoCb) : FFmpeg(filename, AVMEDIA_TYPE_VIDEO), handleVideoData(videoCb) {
    // Setup software scaling context for YUV to RGB conversion
    m_swsContext.reset(sws_getContext(
                m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt,
                m_codecContext->width, m_codecContext->height, AV_PIX_FMT_RGB24,
                SWS_POINT, nullptr, nullptr, nullptr));
}

AudioFFmpeg::AudioFFmpeg(fs::path const& filename, int rate, AudioCb audioCb) :
    FFmpeg(filename, AVMEDIA_TYPE_AUDIO), m_rate(rate), handleAudioData(audioCb) {

        // setup filter to resample, with gain
        createFilter();


        // setup resampler
        m_resampleContext.reset(swr_alloc());
        if (!m_resampleContext) throw std::runtime_error("Cannot create resampling context");
        av_opt_set_int(m_resampleContext.get(), "in_channel_layout", m_codecContext->channel_layout ? static_cast<std::int64_t>(m_codecContext->channel_layout) : av_get_default_channel_layout(m_codecContext->channels), 0);
        av_opt_set_int(m_resampleContext.get(), "out_channel_layout", av_get_default_channel_layout(AUDIO_CHANNELS), 0);
        av_opt_set_int(m_resampleContext.get(), "in_sample_rate", m_codecContext->sample_rate, 0);
        av_opt_set_int(m_resampleContext.get(), "out_sample_rate", static_cast<int>(m_rate), 0);
        av_opt_set_int(m_resampleContext.get(), "in_sample_fmt", m_codecContext->sample_fmt, 0);
        av_opt_set_int(m_resampleContext.get(), "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        swr_init(m_resampleContext.get());
    }

void AudioFFmpeg::createFilter()
{
    // Setup Audio Filter
    //
    // Essentially this needs a source and a sink.
    // The frame-data goes into the source, and is read from the sink
    //
    // Based on: https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/doc/examples/decode_filter_audio.c

    // The filter resamples to m_rate, AUDIO_CHANNELS (1|2)
    // It normalises the human perceived "loudness" to the Relay Gain factor (if any)

    char args[128];  // used by ffmpeg channel describer
    int  ret = 0;
    const AVFilter *abufferSrc  = avfilter_get_by_name("abuffer");           // audio data goes into the filter here
    const AVFilter *abufferSink = avfilter_get_by_name("abuffersink");       // audio data is read out of the filter here

    static const enum AVSampleFormat outSampleFmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
    static const int outSampleRates[] = { static_cast<int>(m_rate), -1 };
    static const char *outSampleLayouts = "stereo";  // Is this OK?  What about 7.1?
    if (AUDIO_CHANNELS == 1)
        outSampleLayouts = "mono";
    else
        std::clog << "ffmpeg/error: Not correctly handling AUDIO_CHANNELS of [" << AUDIO_CHANNELS << "]" << std::endl;  // not mono, not stereo

    // Filter re-samples to Configured Audio Format with Replay Gain "loudness" normalisation
    // Leaving "s16" as it seems to be non-configurable
    char filterStr[128];
    std::snprintf( filterStr,
                   sizeof(filterStr),
                   "aresample=%d,aformat=sample_fmts=s16:channel_layouts=%s,volume=replaygain=track",  // "s16" is AV_SAMPLE_FMT_S16
                   outSampleRates[0],  // e.g.: 48000
                   outSampleLayouts ); // e.g.: "stereo"

    AVFilterInOut *inputs   = avfilter_inout_alloc();   // libav API wants address of this pointers, can't use std::unique_ptr?
    AVFilterInOut *outputs  = avfilter_inout_alloc();
    AVRational     timeBase = m_formatContext.get()->streams[m_streamId]->time_base;

    m_filterGraph.reset( avfilter_graph_alloc() );
    if ( outputs == nullptr || inputs == nullptr || m_filterGraph.get() == nullptr )
    {
        std::clog << "ffmpeg/error: Not enough memory to create filter" << std::endl;
        throw Error(*this, AVERROR(ENOMEM));
    }

    // buffer audio source: the decoded frames from the decoder will be inserted here.
    if ( m_codecContext.get()->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
    {
        av_channel_layout_default(&(m_codecContext.get()->ch_layout), m_codecContext.get()->ch_layout.nb_channels);
    }
    ret = snprintf(args,
                   sizeof(args),
                   "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=",  // channel_layout intentionally blank
                   timeBase.num,
                   timeBase.den, m_codecContext.get()->sample_rate,
                   av_get_sample_fmt_name(m_codecContext.get()->sample_fmt));

    av_channel_layout_describe(&(m_codecContext.get()->ch_layout), args + ret, sizeof(args) - static_cast<size_t>( ret ));
    ret = avfilter_graph_create_filter(&m_filterBufSrc, abufferSrc, "in", args, nullptr, m_filterGraph.get());
    if ( ret < 0 )
    {
        std::clog << "ffmpeg/error: Cannot create audio buffer source" << std::endl;
        throw Error(*this, ret );
    }

    // buffer audio sink: to terminate the filter chain.
    // The filtered audio frames are read from this buffer
    ret = avfilter_graph_create_filter(&m_filterBufSink, abufferSink, "out", nullptr, nullptr, m_filterGraph.get());
    if (ret < 0)
    {
        std::clog << "ffmpeg/error: Cannot create audio buffer sink" << std::endl;
        throw Error(*this, ret );
    }

    //ret = av_opt_set_int_list(m_filterBufSink, "sample_fmts", outSampleFmts, -1, AV_OPT_SEARCH_CHILDREN);  // macro uses mixed result-types
    int size = static_cast<int>(av_int_list_length(outSampleFmts, -1) * sizeof(*(outSampleFmts)));
    ret = av_opt_set_bin(m_filterBufSink, "sample_fmts", (const uint8_t *)(outSampleFmts), size , AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        std::clog << "ffmpeg/error: Cannot set output sample format" << std::endl;
        throw Error(*this, ret );
    }

    ret = av_opt_set(m_filterBufSink, "ch_layouts", outSampleLayouts, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        std::clog << "ffmpeg/error: Cannot set output channel layout" << std::endl;
        throw Error(*this, ret );
    }

    //ret = av_opt_set_int_list(m_filterBufSink, "sample_rates", outSampleRates, -1, AV_OPT_SEARCH_CHILDREN);  // macro uses mixed result-types Grrr!
    size = static_cast<int>(av_int_list_length(outSampleRates, -1) * sizeof(*(outSampleRates)));
    ret = av_opt_set_bin(m_filterBufSink, "sample_rates", (const uint8_t *)(outSampleRates), size, AV_OPT_SEARCH_CHILDREN);

    if (ret < 0)
    {
        std::clog << "ffmpeg/error: Cannot set output sample rate" << std::endl;
        throw Error(*this, ret );
    }

    // Set the endpoints for the filter graph. The filter_graph will
    // be linked to the graph described by filters_descr.

    // The buffer source output must be connected to the input pad of
    // the first filter described by filters_descr; since the first
    // filter input label is not specified, it is set to "in" by
    // default.
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = m_filterBufSrc;
    outputs->pad_idx    = 0;
    outputs->next       = nullptr;

    // The buffer sink input must be connected to the output pad of
    // the last filter described by filters_descr; since the last
    // filter output label is not specified, it is set to "out" by
    // default.
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = m_filterBufSink;
    inputs->pad_idx    = 0;
    inputs->next       = nullptr;

    // Finally we parse the filter string
    int parseRet  = avfilter_graph_parse_ptr(m_filterGraph.get(), filterStr, &inputs, &outputs, nullptr);
    if ( parseRet < 0 )
    {
        std::clog << "ffmpeg/error: Failed to parse filter specification" << std::endl;
        throw Error(*this, parseRet );
    }

    int configRet = avfilter_graph_config(m_filterGraph.get(), nullptr);
    if ( configRet < 0 )
    {
        std::clog << "ffmpeg/error: Failed to configure filter" << std::endl;
        throw Error(*this, configRet );
    }


    // Print summary of the sink buffer
    // Note: args buffer is reused to store channel layout string
    const AVFilterLink *outlink = m_filterBufSink->inputs[0];
    av_channel_layout_describe(&outlink->ch_layout, args, sizeof(args));
    std::clog << "ffmpeg/debug: Filter Output: sample-rate="
              << outlink->sample_rate
              << "Hz  format="
              << (char *)av_x_if_null(av_get_sample_fmt_name( static_cast<enum AVSampleFormat>(outlink->format)), "?")
              << "  channel-layout="
              << args
              << std::endl;


    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
}

double FFmpeg::duration() const { return double(m_formatContext->duration) / double(AV_TIME_BASE); }

double FFmpeg::replayGain() const { return m_replaygain; }

void FFmpeg::avformat_close_input(AVFormatContext *fctx) {
    if (fctx) ::avformat_close_input(&fctx);
}
void FFmpeg::avcodec_free_context(AVCodecContext *avctx) {
    if (avctx == nullptr) return;
    avcodec_close(avctx);
    ::avcodec_free_context(&avctx);
}

void FFmpeg::handleOneFrame() {
    bool read_one = false;
    do {
        std::unique_ptr<AVPacket, std::function<void(AVPacket*)>> pkt(av_packet_alloc(), [] (auto *pkt) { av_packet_free(&pkt); });
        // GET THE FRAME
        auto ret = av_read_frame(m_formatContext.get(), pkt.get());  // Read a frame of the stream

        if(ret == AVERROR_EOF) {
            // End of file: no more data to read.
            throw Eof();
        } else if(ret < 0) {
            throw Error(*this, ret);
        }

        if (pkt->stream_index != m_streamId) // Is the packet from the stream we want?
            continue;

        ret = avcodec_send_packet(m_codecContext.get(), pkt.get()); // send the packet off to the decoder, to make into frames

        if(ret == AVERROR_EOF) {
                // End of file: no more data to read.
                throw Eof();
        } else if(ret == AVERROR(EAGAIN)) {
                // no room for new data, need to get more frames out of the decoder by
                // calling avcodec_receive_frame()
        } else if(ret < 0) {
                throw Error(*this, ret);
        }

        handleSomeFrames();
        read_one = true;

    } while (!read_one);
}

void FFmpeg::seek(double time) {
    // AVSEEK_FLAG_BACKWARD makes sure we always get a keyframe BEFORE the
    // request time, thus it allows us to drop some frames to reach the
    // exact point where asked to seek
    int flags = AVSEEK_FLAG_BACKWARD;
    av_seek_frame(m_formatContext.get(), -1, static_cast<std::int64_t>(time * AV_TIME_BASE), flags);
}

void AudioFFmpeg::seek(double time) {
    FFmpeg::seek(time);
    m_position_in_48k_frames = -1; //kill previous position
}

void FFmpeg::handleSomeFrames()
{
    int ret;
    do {
        uFrame frame{av_frame_alloc()};

        ret = avcodec_receive_frame(m_codecContext.get(), frame.get());  // Fetch a frame from the packet decoder

        if (ret == AVERROR_EOF) {
            // End of file: no more data.
            throw Eof();
        } else if (ret == AVERROR(EAGAIN)) {
            // not enough data to decode a frame, go read more and feed more to the decoder
            break;
        } else if (ret < 0) {
            throw Error(*this, ret);
        }

        // frame is available here
        if (frame->pts != std::int64_t(AV_NOPTS_VALUE))
        {
            auto new_position = double(frame->pts) * av_q2d(m_formatContext->streams[m_streamId]->time_base);
            if (m_formatContext->streams[m_streamId]->start_time != std::int64_t(AV_NOPTS_VALUE))
                new_position -= double(m_formatContext->streams[m_streamId]->start_time) * av_q2d(m_formatContext->streams[m_streamId]->time_base);
            m_position = new_position;
        }

        processFrame(std::move(frame));

    } while (ret >= 0);
}

void VideoFFmpeg::processFrame(uFrame frame) {
    // Convert into RGB and scale the data
    auto w = static_cast<unsigned>((m_codecContext->width + 15) & ~15);
    auto h = static_cast<unsigned>(m_codecContext->height);
    Bitmap f;
    f.timestamp = m_position;
    f.fmt = pix::Format::RGB;
    f.resize(w, h);
    {
        std::uint8_t* data = f.data();
        int linesize = static_cast<int>(w * 3);
        sws_scale(m_swsContext.get(), frame->data, frame->linesize, 0, static_cast<int>(h), &data, &linesize);
    }
    handleVideoData(std::move(f));  // Takes ownership and may block until there is space
}

void AudioFFmpeg::processFrameOld(uFrame frame) {
    // resample to output
    std::int16_t *output;
    int out_samples = swr_get_out_samples(m_resampleContext.get(), frame->nb_samples);
    av_samples_alloc((std::uint8_t**)&output, nullptr, AUDIO_CHANNELS, out_samples,
            AV_SAMPLE_FMT_S16, 0);
    out_samples = swr_convert(m_resampleContext.get(), (std::uint8_t**)&output, out_samples,
            (const std::uint8_t**)&frame->data[0], frame->nb_samples);
    // The output is now an interleaved array of 16-bit samples
    if (m_position_in_48k_frames == -1) {
        m_position_in_48k_frames = static_cast<std::int64_t>(m_position * m_rate + 0.5f);
    }
    handleAudioData(output, out_samples * AUDIO_CHANNELS, m_position_in_48k_frames * AUDIO_CHANNELS /* pass in samples */);
    av_freep(&output);
    m_position_in_48k_frames += out_samples;
    m_position += frame->nb_samples * av_q2d(m_formatContext->streams[m_streamId]->time_base);
}

void AudioFFmpeg::processFrame(uFrame frame) {
    int ret;
    AVFrame *filt_frame = av_frame_alloc();  // TODO: can we use a qtd::unique_ptr<> for this ?

    // push the audio data from decoded frame into the filtergraph
    if (av_buffersrc_add_frame_flags(m_filterBufSrc, frame.get(), AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
        std::clog << "ffmpeg/error: Error while feeding the audio filtergraph" << std::endl;
        throw Error(*this, -1 );
    }

    // Try to read frame(s) out of the filter
    while (1) {
        ret = av_buffersink_get_frame(m_filterBufSink, filt_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            break;
        }
        else if (ret < 0)
        {
            std::clog << "ffmpeg/error: Error getting frame from filter" << std::endl;
            throw Error(*this, ret );
        }

        // Get the samples from the filtered frame, they do not need resampling
        std::int16_t *output;
        int out_samples = filt_frame->nb_samples;
        //av_samples_alloc((std::uint8_t**)&output, nullptr, AUDIO_CHANNELS, out_samples, AV_SAMPLE_FMT_S16, 0);
        //av_samples_copy( (std::uint8_t**)&output, (std::uint8_t**)&(filt_frame->data), 0, 0, out_samples, AUDIO_CHANNELS,  AV_SAMPLE_FMT_S16 );
        output = (std::int16_t*)filt_frame->data;

        std::clog << "ffmpeg/debug: fetched [" << out_samples << "] samples, of [" << filt_frame->ch_layout.nb_channels << "] channels from filter" << std::endl;

        std::int64_t amount = std::min( out_samples, 32 );
        std::stringstream msg;
        msg << "Audio Data is [";
        for ( ssize_t i=0; i<amount; i++)
        {
            msg << output[i];
            if ( i <amount-1 )
                msg << ", ";
        }
        msg << "] ...";
        std::clog << "ffmpeg/debug: " << msg.str() << "]" << std::endl;


        // The output is now an interleaved array of 16-bit samples
        if (m_position_in_48k_frames == -1) {
            m_position_in_48k_frames = static_cast<std::int64_t>(m_position * m_rate + 0.5f);
        }
        handleAudioData(output, out_samples * AUDIO_CHANNELS, m_position_in_48k_frames * AUDIO_CHANNELS /* pass in samples */);
        //av_freep(&output);
        m_position_in_48k_frames += out_samples;
        m_position += filt_frame->nb_samples * av_q2d(m_formatContext->streams[m_streamId]->time_base);

        av_frame_unref(filt_frame);

    }
    av_frame_free(&filt_frame);
    av_frame_unref(frame.get());
}



#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
