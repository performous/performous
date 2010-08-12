#include <libda/audio_dev.hpp>
#include <alsa/alsa.hpp>
#include <boost/lexical_cast.hpp>
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

#include <iostream>

namespace {
	using namespace da;


	void devices(snd_pcm_stream_t stream) {
		alsa::ctl_card_info info;
		for (int card = -1; ALSA_CHECKED(snd_card_next, (&card)), card >= 0;) {
			alsa::ctl ctl(("hw:" + boost::lexical_cast<std::string>(card)).c_str());
			snd_ctl_card_info(ctl, info);
			for (int dev = -1; ALSA_CHECKED(snd_ctl_pcm_next_device, (ctl, &dev)), dev >= 0;) {
				alsa::pcm_info pcminfo(dev, 0, stream);
				if (snd_ctl_pcm_info(ctl, pcminfo) < 0) continue;
				std::string device = std::string("alsa:hw:") + snd_ctl_card_info_get_id(info) + "," + boost::lexical_cast<std::string>(dev);
				std::string desc = std::string(snd_ctl_card_info_get_name(info)) + " (" + snd_pcm_info_get_name(pcminfo) + ")";
				std::cout << "  " << device << "   " << desc << std::endl;
			}
		}
	}

	struct Foo {
		Foo() {
			std::cout << "ALSA capture devices:\n";
			devices(SND_PCM_STREAM_CAPTURE);
			std::cout << "ALSA playback devices:\n";
			devices(SND_PCM_STREAM_PLAYBACK);
			std::cout << std::endl;
		}
	} foo;

	/** Configure a pcm by the settings, store actual values back to s. Return the chose sample format. **/
	snd_pcm_format_t config(alsa::pcm& pcm, settings& s) {
		// Convert settings into types used by ALSA
		unsigned int rate = s.rate();
		unsigned int channels = s.channels();
		if (s.frames() == settings::low) s.set_frames(256);
		else if (s.frames() == settings::high) s.set_frames(16384);
		snd_pcm_uframes_t period_size = s.frames();
		snd_pcm_uframes_t buffer_size = 0;
		alsa::hw_config hw(pcm);
		hw.set(SND_PCM_ACCESS_MMAP_INTERLEAVED);
		if (s.rate() == settings::high) hw.rate_last(rate);
		else if (s.rate() == settings::low) hw.rate_first(rate);
		else if (s.rate_near()) hw.rate_near(rate);
		else hw.rate(rate);
		if (s.channels() == settings::high) hw.channels_last(channels);
		else if (s.channels() == settings::low) hw.channels_first(channels);
		else if (s.channels_near()) hw.channels_near(channels);
		else hw.channels(channels);
		hw.period_size_near(period_size).buffer_size_near(buffer_size = 4 * period_size);
		// TODO: move S16 as the last option. It is the second one now to avoid ALSA bug in that mode (cracking with INT_MIN-valued samples).
		snd_pcm_format_t fmt[] = { SND_PCM_FORMAT_FLOAT, SND_PCM_FORMAT_S16, SND_PCM_FORMAT_S32, SND_PCM_FORMAT_S24_3LE };
		size_t fmt_size = sizeof(fmt) / sizeof(*fmt);
		size_t i = 0;
		alsa::hw_params backup = hw;
		while (i < fmt_size) {
			try { hw.set(fmt[i]).commit(); break; } catch (alsa::error const&) { if (++i == fmt_size) throw; hw.load(backup); }
		}
		// Assign the changed settings back
		s.set_channels(channels);
		s.set_rate(rate);
		s.set_frames(period_size);
		if (s.subdev().empty()) s.set_subdev("default");
		return fmt[i];
	}

	/*
	void status(alsa::pcm& pcm) {
		switch (snd_pcm_state(pcm)) {
		  case SND_PCM_STATE_OPEN: std::clog << "ALSA pcm state: OPEN" << std::endl; break;
		  case SND_PCM_STATE_SETUP: std::clog << "ALSA pcm state: SETUP" << std::endl; break;
		  case SND_PCM_STATE_PREPARED: std::clog << "ALSA pcm state: PREPARED" << std::endl; break;
		  case SND_PCM_STATE_RUNNING: std::clog << "ALSA pcm state: RUNNING" << std::endl; break;
		  case SND_PCM_STATE_XRUN: std::clog << "ALSA pcm state: XRUN" << std::endl; break;
		  case SND_PCM_STATE_DRAINING: std::clog << "ALSA pcm state: DRAINING" << std::endl; break;
		  case SND_PCM_STATE_PAUSED: std::clog << "ALSA pcm state: PAUSED" << std::endl; break;
		  case SND_PCM_STATE_SUSPENDED: std::clog << "ALSA pcm state: SUSPENDED" << std::endl; break;
		  case SND_PCM_STATE_DISCONNECTED: std::clog << "ALSA pcm state: DISCONNECTED" << std::endl; break;
		}
	}
	*/

	class alsa_record: public record::dev {
		settings m_s;
		alsa::pcm m_pcm;
		volatile bool m_quit;
		boost::scoped_ptr<boost::thread> m_thread;
		snd_pcm_format_t m_fmt;
	  public:
		alsa_record(settings& s):
		  m_s(s),
		  m_pcm(s.subdev().empty() ? "default" : m_s.subdev().c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK),
		  m_quit(false),
		  m_fmt(config(m_pcm, m_s))
		{
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
						const unsigned int samplebits = snd_pcm_format_width(m_fmt);
						for (std::size_t ch = 0; ch < channels; ++ch) {
							snd_pcm_channel_area_t const& a = mmap.area(ch);
							if (a.first % samplebits || a.step % samplebits) throw std::runtime_error("The sample alignment used by snd_pcm_mmap not supported by audio::alsa_record");
						}
						for (snd_pcm_uframes_t fr = 0; fr < mmap.frames(); ++fr) {
							for (std::size_t ch = 0; ch < channels; ++ch) {
								snd_pcm_channel_area_t const& a = mmap.area(ch);
								if (m_fmt == SND_PCM_FORMAT_FLOAT) {
									buf[fr * channels + ch] = static_cast<float*>(a.addr)[(a.first + fr * a.step) / samplebits + mmap.offset() * channels];
								} else if (m_fmt == SND_PCM_FORMAT_S16) {
									const int sample = static_cast<int16_t*>(a.addr)[(a.first + fr * a.step) / samplebits + mmap.offset() * channels];
									buf[fr * channels + ch] = conv_from_s16(sample);
								} else if (m_fmt == SND_PCM_FORMAT_S32) {
									const int sample = static_cast<int32_t*>(a.addr)[(a.first + fr * a.step) / samplebits + mmap.offset() * channels];
									buf[fr * channels + ch] = conv_from_s32(sample);
								} else if (m_fmt == SND_PCM_FORMAT_S24_3LE) {
									unsigned char* data = static_cast<unsigned char*>(a.addr) + 3 * ((a.first + fr * a.step) / samplebits + mmap.offset() * channels);
									int32_t s = data[0] << 8 | data[1] << 16 | data[2] << 24;
									buf[fr * channels + ch] = conv_from_s24(s >> 8);
								} else throw std::logic_error("The sample format chosen is not supported by alsa_record (internal error)");
							}
						}
						mmap.commit();
					}
					pcm_data data(&buf[0], buf.size() / channels, channels, m_s.rate());
					try {
						m_s.callback()(data);
					} catch (std::exception& e) {
						m_s.debug(std::string("Exception from recording callback: ") + e.what());
					}
				} catch (alsa::error const& e) {
					if (e.code() != -EPIPE) m_s.debug(std::string("Recording error: ") + e.what());
					int err = snd_pcm_recover(m_pcm, e.code(), 0);
					if (err < 0) m_s.debug(std::string("ALSA snd_pcm_recover failed: ") + snd_strerror(err));
					if (snd_pcm_start(m_pcm) < 0) m_s.debug("Unable to restart the recording stream!");
				} catch (std::exception const& e) {
					m_s.debug(std::string("Recording error: ") + e.what());
				}
			}
		}
	};
	plugin::simple<record_plugin, alsa_record> r(devinfo("alsa", "ALSA PCM capture. Device string can be given as settings (default is \"default\")."));

	class alsa_playback: public playback::dev {
		settings m_s;
		alsa::pcm m_pcm;
		volatile bool m_quit;
		boost::scoped_ptr<boost::thread> m_thread;
		snd_pcm_format_t m_fmt;
	  public:
		alsa_playback(settings& s):
		  m_s(s),
		  m_pcm(s.subdev().empty() ? "default" : m_s.subdev().c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK),
		  m_quit(false),
		  m_fmt(config(m_pcm, m_s))
		{
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
				pcm_data data(&buf[0], buf.size() / channels, channels, m_s.rate());
				try {
					m_s.callback()(data);
				} catch (std::exception& e) {
					m_s.debug(std::string("Exception from playback callback: ") + e.what());
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
						const unsigned int samplebits = snd_pcm_format_width(m_fmt);
						for (std::size_t ch = 0; ch < channels; ++ch) {
							snd_pcm_channel_area_t const& a = mmap.area(ch);
							if (a.first % samplebits || a.step % samplebits)
							  throw std::runtime_error("The sample alignment used by snd_pcm_mmap not supported by audio::alsa_record");
						}
						for (snd_pcm_uframes_t fr = 0; fr < frames; ++fr) {
							for (std::size_t ch = 0; ch < channels; ++ch) {
								snd_pcm_channel_area_t const& a = mmap.area(ch);
								if (m_fmt == SND_PCM_FORMAT_FLOAT) {
									static_cast<float*>(a.addr)[(a.first + fr * a.step) / samplebits + mmap.offset() * channels] = buf[(pos + fr) * channels + ch];
								} else if (m_fmt == SND_PCM_FORMAT_S16) {
									static_cast<int16_t*>(a.addr)[(a.first + fr * a.step) / samplebits + mmap.offset() * channels] = conv_to_s16(buf[(pos + fr) * channels + ch]);
								} else if (m_fmt == SND_PCM_FORMAT_S32) {
									static_cast<int32_t*>(a.addr)[(a.first + fr * a.step) / samplebits + mmap.offset() * channels] = conv_to_s32(buf[(pos + fr) * channels + ch]);
								} else if (m_fmt == SND_PCM_FORMAT_S24_3LE) {
									unsigned char* data = static_cast<unsigned char*>(a.addr) + 3 * ((a.first + fr * a.step) / samplebits + mmap.offset() * channels);
									const uint32_t s = conv_to_s24(buf[(pos + fr) * channels + ch]);
									data[0] = s; data[1] = s >> 8; data[2] = s >> 16;
								} else throw std::logic_error("The sample format chosen is not supported by alsa_playback (internal error)");
							}
						}
						mmap.commit(frames);
						pos += frames;
					}
					if (first) { ALSA_CHECKED(snd_pcm_start, (m_pcm)); first = false; } // For playback, this must be done after filling the buffer
				} catch (alsa::error const& e) {
					if (e.code() != -EPIPE) m_s.debug(std::string("Playback error: ") + e.what());
					int err = snd_pcm_recover(m_pcm, e.code(), 0);
					if (err < 0) m_s.debug(std::string("ALSA snd_pcm_recover failed: ") + snd_strerror(err));
					first = true;
				} catch (std::exception const& e) {
					m_s.debug(std::string("Playback error: ") + e.what());
				}
			}
		}
	};
	plugin::simple<playback_plugin, alsa_playback> p(devinfo("alsa", "ALSA PCM playback. Device string can be given as settings (default is \"default\")."));
}


