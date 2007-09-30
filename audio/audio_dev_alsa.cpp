#include "audio_dev.hpp"
#include <alsa/alsa.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <algorithm>
#include <ostream>
#include <stdint.h>

namespace da {
	class alsa_record: public record::dev {
		static reg_dev reg;
		static dev* create(settings& s) { return new alsa_record(s); }
		settings s;
		alsa::pcm pcm;
		volatile bool quit;
		boost::scoped_ptr<boost::thread> thread;
	  public:
		alsa_record(settings& s_orig): s(s_orig), pcm(s.subdev.empty() ? "default" : s.subdev.c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK), quit(false) {
			// Convert settings into types used by ALSA
			unsigned int rate = s.rate;
			unsigned int channels = s.channels;
			if (s.frames == settings::low) s.frames = 256;
			if (s.frames == settings::high) s.frames = 16384;
			snd_pcm_uframes_t period_size = s.frames;
			snd_pcm_uframes_t buffer_size = 0;
			alsa::hw_config hw(pcm);
			hw.set(SND_PCM_ACCESS_MMAP_INTERLEAVED).set(SND_PCM_FORMAT_S16_LE);
			if (s.rate == settings::high) hw.rate_last(rate);
			else if (s.rate == settings::low) hw.rate_first(rate);
			else if (s.rate_near) hw.rate_near(rate);
			else hw.rate(rate);
			if (s.channels == settings::high) hw.channels_last(channels);
			else if (s.channels == settings::low) hw.channels_first(channels);
			else if (s.channels_near) hw.channels_near(channels);
			else hw.channels(channels);
			hw.period_size_near(period_size)
			  .buffer_size_near(buffer_size = 4 * period_size)
			  .commit();
			// Assign the changed settings back
			s.channels = channels;
			s.rate = rate;
			s.frames = period_size;
			thread.reset(new boost::thread(boost::ref(*this)));
			ALSA_CHECKED(snd_pcm_start, (pcm));
			// Return the chosen settings
			s_orig = s;
		}
		~alsa_record() {
			quit = true;
			thread->join();
		}
		void operator()() {
			std::vector<sample_t> buf;
			while (!quit) {
				try {
					// Sleep until samples are available
					ALSA_CHECKED(snd_pcm_wait, (pcm, 1000));
					// Request samples by MMAP, convert and copy them to buf
					{
						ALSA_CHECKED(snd_pcm_avail_update, (pcm));
						alsa::mmap mmap(pcm, s.frames);
						buf.resize(mmap.frames * s.channels);
						// TODO: bytewise copy (when needed, e.g. 24 bit samples)
						const unsigned int samplebits = 8 * sizeof(int16_t);
						for (unsigned int ch = 0; ch < s.channels; ++ch) {
							snd_pcm_channel_area_t const& a = mmap.areas[ch];
							if (a.first % samplebits || a.step % samplebits)
							  throw std::runtime_error("The sample alignment used by snd_pcm_mmap not supported by audio::alsa_record");
						}
						for (snd_pcm_uframes_t fr = 0; fr < mmap.frames; ++fr) {
							for (unsigned int ch = 0; ch < s.channels; ++ch) {
								snd_pcm_channel_area_t const& a = mmap.areas[ch];
								const int sample = static_cast<int16_t*>(a.addr)[(a.first + fr * a.step) / samplebits + mmap.offset * s.channels];
								buf[fr * s.channels + ch] = conv_from_s16(sample);
							}
						}
					}
					pcm_data data(&buf[0], buf.size() / s.channels, s.channels);
					try {
						s.callback(data, s);
					} catch (std::exception& e) {
						if (s.debug) *s.debug << "Exception from recording callback: " << e.what() << std::endl;
					}
				} catch (alsa::error& e) {
					if (s.debug) {
						if (e.code() != -EPIPE) *s.debug << "Recording error: " << e.what() << std::endl;
					}
					int err = snd_pcm_recover(pcm, e.code(), 0);
					if (err < 0 && s.debug) *s.debug << "ALSA snd_pcm_recover failed: " << snd_strerror(err) << std::endl;
					if (snd_pcm_start(pcm) < 0 && s.debug) *s.debug << "Unable to restart the recording stream!" << std::endl;
				}
			}
		}
	};

	alsa_record::reg_dev alsa_record::reg("alsa", alsa_record::create);
}

