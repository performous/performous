#include "audio_dev.hpp"
#include <alsa/alsa.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <algorithm>
#include <ostream>
#include <stdint.h>

#if SND_LIB_VERSION < (1<<16 | 0<<8 | 11)
#  error "######################################################"
#  error "###  Please install at least alsa-lib 1.0.11"
#  error "######################################################"
#endif


namespace {
	using namespace da;
	class alsa_record: public record::dev {
		settings m_s;
		alsa::pcm m_pcm;
		volatile bool m_quit;
		boost::scoped_ptr<boost::thread> m_thread;
	  public:
		alsa_record(settings& s):
		  m_s(s),
		  m_pcm(s.subdev().empty() ? "default" : m_s.subdev().c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK),
		  m_quit(false)
		{
			// Convert settings into types used by ALSA
			unsigned int rate = m_s.rate();
			unsigned int channels = m_s.channels();
			if (m_s.frames() == settings::low) m_s.set_frames(256);
			else if (m_s.frames() == settings::high) m_s.set_frames(16384);
			snd_pcm_uframes_t period_size = m_s.frames();
			snd_pcm_uframes_t buffer_size = 0;
			alsa::hw_config hw(m_pcm);
			hw.set(SND_PCM_ACCESS_MMAP_INTERLEAVED).set(SND_PCM_FORMAT_S16);
			if (m_s.rate() == settings::high) hw.rate_last(rate);
			else if (m_s.rate() == settings::low) hw.rate_first(rate);
			else if (m_s.rate_near()) hw.rate_near(rate);
			else hw.rate(rate);
			if (m_s.channels() == settings::high) hw.channels_last(channels);
			else if (m_s.channels() == settings::low) hw.channels_first(channels);
			else if (m_s.channels_near()) hw.channels_near(channels);
			else hw.channels(channels);
			hw.period_size_near(period_size)
			  .buffer_size_near(buffer_size = 4 * period_size)
			  .commit();
			// Assign the changed settings back
			m_s.set_channels(channels);
			m_s.set_rate(rate);
			m_s.set_frames(period_size);
			m_thread.reset(new boost::thread(boost::ref(*this)));
			ALSA_CHECKED(snd_pcm_start, (m_pcm));
			s = m_s;
		}
		~alsa_record() {
			m_quit = true;
			m_thread->join();
		}
		void operator()() {
			std::vector<sample_t> buf;
			while (!m_quit) {
				try {
					const std::size_t channels = m_s.channels();
					// Sleep until samples are available
					ALSA_CHECKED(snd_pcm_wait, (m_pcm, 1000));
					// Request samples by MMAP, convert and copy them to buf
					{
						ALSA_CHECKED(snd_pcm_avail_update, (m_pcm));
						alsa::mmap mmap(m_pcm, m_s.frames());
						buf.resize(mmap.frames * m_s.channels());
						// TODO: bytewise copy (when needed, e.g. 24 bit samples)
						const unsigned int samplebits = 8 * sizeof(int16_t);
						for (std::size_t ch = 0; ch < channels; ++ch) {
							snd_pcm_channel_area_t const& a = mmap.areas[ch];
							if (a.first % samplebits || a.step % samplebits)
							  throw std::runtime_error("The sample alignment used by snd_pcm_mmap not supported by audio::alsa_record");
						}
						for (snd_pcm_uframes_t fr = 0; fr < mmap.frames; ++fr) {
							for (std::size_t ch = 0; ch < channels; ++ch) {
								snd_pcm_channel_area_t const& a = mmap.areas[ch];
								const int sample = static_cast<int16_t*>(a.addr)[(a.first + fr * a.step) / samplebits + mmap.offset * channels];
								buf[fr * channels + ch] = conv_from_s16(sample);
							}
						}
					}
					pcm_data data(&buf[0], buf.size() / channels, channels);
					try {
						m_s.callback()(data, m_s);
					} catch (std::exception& e) {
						m_s.debug(std::string("Exception from recording callback: ") + e.what());
					}
				} catch (alsa::error& e) {
					if (e.code() != -EPIPE) m_s.debug(std::string("Recording error: ") + e.what());
					int err = snd_pcm_recover(m_pcm, e.code(), 0);
					if (err < 0) m_s.debug(std::string("ALSA snd_pcm_recover failed: ") + snd_strerror(err));
					if (snd_pcm_start(m_pcm) < 0) m_s.debug("Unable to restart the recording stream!");
				}
			}
		}
	};
	boost::plugin::simple<record_plugin, alsa_record> r(devinfo("alsa", "ALSA PCM capture. Device string can be given as settings (default is \"default\")."));
}


