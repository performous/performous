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
	/** Configure a pcm by the settings, store actual values back to s. **/
	void config(alsa::pcm& pcm, settings& s) {
		// Convert settings into types used by ALSA
		unsigned int rate = s.rate();
		unsigned int channels = s.channels();
		if (s.frames() == settings::low) s.set_frames(256);
		else if (s.frames() == settings::high) s.set_frames(16384);
		snd_pcm_uframes_t period_size = s.frames();
		snd_pcm_uframes_t buffer_size = 0;
		alsa::hw_config hw(pcm);
		hw.set(SND_PCM_ACCESS_MMAP_INTERLEAVED).set(SND_PCM_FORMAT_S16);
		if (s.rate() == settings::high) hw.rate_last(rate);
		else if (s.rate() == settings::low) hw.rate_first(rate);
		else if (s.rate_near()) hw.rate_near(rate);
		else hw.rate(rate);
		if (s.channels() == settings::high) hw.channels_last(channels);
		else if (s.channels() == settings::low) hw.channels_first(channels);
		else if (s.channels_near()) hw.channels_near(channels);
		else hw.channels(channels);
		hw.period_size_near(period_size)
		  .buffer_size_near(buffer_size = 4 * period_size)
		  .commit();
		// Assign the changed settings back
		s.set_channels(channels);
		s.set_rate(rate);
		s.set_frames(period_size);
	}

	void status(alsa::pcm& pcm) {
		switch (snd_pcm_state(pcm)) {
		  case SND_PCM_STATE_OPEN: std::cout << "ALSA pcm state: OPEN" << std::endl; break;
		  case SND_PCM_STATE_SETUP: std::cout << "ALSA pcm state: SETUP" << std::endl; break;
		  case SND_PCM_STATE_PREPARED: std::cout << "ALSA pcm state: PREPARED" << std::endl; break;
		  case SND_PCM_STATE_RUNNING: std::cout << "ALSA pcm state: RUNNING" << std::endl; break;
		  case SND_PCM_STATE_XRUN: std::cout << "ALSA pcm state: XRUN" << std::endl; break;
		  case SND_PCM_STATE_DRAINING: std::cout << "ALSA pcm state: DRAINING" << std::endl; break;
		  case SND_PCM_STATE_PAUSED: std::cout << "ALSA pcm state: PAUSED" << std::endl; break;
		  case SND_PCM_STATE_SUSPENDED: std::cout << "ALSA pcm state: SUSPENDED" << std::endl; break;
		  case SND_PCM_STATE_DISCONNECTED: std::cout << "ALSA pcm state: DISCONNECTED" << std::endl; break;
		}
	}
		
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
			config(m_pcm, m_s);
			ALSA_CHECKED(snd_pcm_start, (m_pcm));  // For recording this must be done before starting
			m_thread.reset(new boost::thread(boost::ref(*this)));
			s = m_s;
		}
		~alsa_record() {
			m_quit = true;
			m_thread->join();
		}
		void operator()() {
			std::vector<sample_t> buf;
			bool first = true;
			while (!m_quit) {
				try {
					const std::size_t channels = m_s.channels();
					// Sleep until samples are available
					ALSA_CHECKED(snd_pcm_wait, (m_pcm, 1000));
					// Request samples by MMAP, convert and copy them to buf
					{
						ALSA_CHECKED(snd_pcm_avail_update, (m_pcm));
						alsa::mmap mmap(m_pcm, m_s.frames());
						buf.resize(mmap.frames() * channels);
						// TODO: bytewise copy (when needed, e.g. 24 bit samples)
						const unsigned int samplebits = 8 * sizeof(int16_t);
						for (std::size_t ch = 0; ch < channels; ++ch) {
							snd_pcm_channel_area_t const& a = mmap.area(ch);
							if (a.first % samplebits || a.step % samplebits)
							  throw std::runtime_error("The sample alignment used by snd_pcm_mmap not supported by audio::alsa_record");
						}
						for (snd_pcm_uframes_t fr = 0; fr < mmap.frames(); ++fr) {
							for (std::size_t ch = 0; ch < channels; ++ch) {
								snd_pcm_channel_area_t const& a = mmap.area(ch);
								const int sample = static_cast<int16_t*>(a.addr)[(a.first + fr * a.step) / samplebits + mmap.offset() * channels];
								buf[fr * channels + ch] = conv_from_s16(sample);
							}
						}
						mmap.commit();
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

	class alsa_playback: public playback::dev {
		settings m_s;
		alsa::pcm m_pcm;
		volatile bool m_quit;
		boost::scoped_ptr<boost::thread> m_thread;
	  public:
		alsa_playback(settings& s):
		  m_s(s),
		  m_pcm(s.subdev().empty() ? "default" : m_s.subdev().c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK),
		  m_quit(false)
		{
			config(m_pcm, m_s);
			m_thread.reset(new boost::thread(boost::ref(*this)));
			s = m_s;
		}
		~alsa_playback() {
			m_quit = true;
			m_thread->join();
		}
		void operator()() {
			std::vector<sample_t> buf;
			bool first = true;
			while (!m_quit) {
				const std::size_t channels = m_s.channels();
				// Request data from application
				buf.resize(m_s.frames() * channels);
				pcm_data data(&buf[0], buf.size() / channels, channels);
				try {
					m_s.callback()(data, m_s);
				} catch (std::exception& e) {
					m_s.debug(std::string("Exception from recording callback: ") + e.what());
				}
				try {
					// Sleep until samples are available
					ALSA_CHECKED(snd_pcm_wait, (m_pcm, 1000));
					// Request samples by MMAP, convert and copy them to buf
					for (size_t pos = 0; pos < m_s.frames();) {
						if (m_quit) return;
						ALSA_CHECKED(snd_pcm_avail_update, (m_pcm));
						alsa::mmap mmap(m_pcm, m_s.frames());
						// How many frames can we send = min(mmap size, input buffer left)
						size_t frames = std::min<size_t>(mmap.frames(), pos - m_s.frames());
						// TODO: bytewise copy (when needed, e.g. 24 bit samples)
						const unsigned int samplebits = 8 * sizeof(int16_t);
						for (std::size_t ch = 0; ch < channels; ++ch) {
							snd_pcm_channel_area_t const& a = mmap.area(ch);
							if (a.first % samplebits || a.step % samplebits)
							  throw std::runtime_error("The sample alignment used by snd_pcm_mmap not supported by audio::alsa_record");
						}
						for (snd_pcm_uframes_t fr = 0; fr < frames; ++fr) {
							for (std::size_t ch = 0; ch < channels; ++ch) {
								snd_pcm_channel_area_t const& a = mmap.area(ch);
								static_cast<int16_t*>(a.addr)[(a.first + fr * a.step) / samplebits + mmap.offset() * channels] = conv_to_s16(buf[(pos + fr) * channels + ch]);
							}
						}
						mmap.commit(frames);
						pos += frames;
					}
					if (first) { ALSA_CHECKED(snd_pcm_start, (m_pcm)); first = false; } // For playback, this must be done after filling the buffer
				} catch (alsa::error& e) {
					if (e.code() != -EPIPE) m_s.debug(std::string("Playback error: ") + e.what());
					int err = snd_pcm_recover(m_pcm, e.code(), 0);
					if (err < 0) m_s.debug(std::string("ALSA snd_pcm_recover failed: ") + snd_strerror(err));
					first = true;
				} catch (std::exception& e) {
					m_s.debug(std::string("Playback error: ") + e.what());
				}
			}
		}
	};
	boost::plugin::simple<playback_plugin, alsa_playback> p(devinfo("alsa", "ALSA PCM playback. Device string can be given as settings (default is \"default\")."));
}


