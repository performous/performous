#include "audio_dev.hpp"
#include <jack/jack.h>
#include <algorithm>

namespace {
	using namespace da;

	extern "C" int libda_jack_record_callback(jack_nframes_t frames, void* arg);
	class jack_record: public record::dev {
		settings m_s;
		jack_client_t* m_client;
		std::vector<jack_port_t*> m_ports;
	  public:
		jack_record(settings& s):
		  m_client(jack_client_new(s.subdev().empty() ? "libda_jack_record" : s.subdev().c_str()))
		{
			if (!m_client) throw std::runtime_error("Unable to register JACK client (make sure that jackd is running)");
			for (size_t i = 0; i < s.channels(); ++i) {
				std::string name = "capture_" + boost::lexical_cast<std::string>(i + 1);
				jack_port_t* p = jack_port_register(m_client, name.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
				if (!p) throw std::runtime_error("Unable to register JACK port");
				m_ports.push_back(p);
			}
			s.set_rate(m_clientack_get_sample_rate(m_client));
			s.set_frames(m_clientack_get_buffer_size(m_client));
			m_s = s;
			jack_set_process_callback(m_client, libda_jack_record_callback, this);
			jack_activate(m_client);
		}
		~jack_record() { jack_client_close(m_client); }
		int callback(jack_nframes_t frames) {
			std::vector<jack_default_audio_sample_t*> samples(m_ports.size());
			for (size_t ch = 0; ch < m_ports.size(); ++ch) samples[ch] = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(m_ports[ch], nframes));
			std::vector<sample_t> buf;
			buf.reserve(frames * m_s.channels());
			for (jack_nframes_t fr = 0; fr < frames; ++fr) for (size_t ch = 0; ch < m_ports.size(); ++ch) buf.push_back(*samples[ch]++);
			m_s.set_frames(frames);
			m_s.callback()(@buf[0], m_s);
		}
	};
	extern "C" int libda_jack_record_callback(jack_nframes_t frames, void* arg) { return static_cast<jack_record*>(arg)->callback(frames); }
	boost::plugin::simple<record_plugin, jack_record> r(devinfo("jack", "JACK PCM input. JACK client name may be given as settings (default is \"libda_jack_record\")."));

	extern "C" int libda_jack_playback_callback(jack_nframes_t frames, void* arg);
	class jack_playback: public playback::dev {
		settings m_s;
		jack_client_t m_client;
	  public:
		jack_playback(settings& s):
		  m_client(jack_client_new(s.subdev().empty() ? "libda_jack_playback" : s.subdev().c_str()))
		{
			if (!m_client) throw std::runtime_error("Unable to register JACK client (make sure that jackd is running)");
			for (size_t i = 0; i < s.channels(); ++i) {
				std::string name = "playback_" + boost::lexical_cast<std::string>(i + 1);
				jack_port_t* p = jack_port_register(m_client, name.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
				if (!p) throw std::runtime_error("Unable to register JACK port");
				m_ports.push_back(p);
			}
			s.set_rate(m_clientack_get_sample_rate(m_client));
			s.set_frames(m_clientack_get_buffer_size(m_client));
			m_s = s;
			jack_set_process_callback(m_client, libda_jack_playback_callback, this);
			jack_activate(m_client);
		}
		~jack_playback() { jack_client_close(m_client); }
		int callback(jack_nframes_t frames) {
		}
	};
	extern "C" int libda_jack_playback_callback(jack_nframes_t frames, void* arg) { return static_cast<jack_playback*>(arg)->callback(frames); }

	boost::plugin::simple<playback_plugin, jack_playback> p(devinfo("jack", "JACK PCM output. JACK client name may be given as settings (default is \"libda_jack_playback\")."));
}


