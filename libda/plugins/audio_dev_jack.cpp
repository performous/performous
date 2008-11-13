#include <libda/plugins/audio_dev.hpp>
#include <boost/lexical_cast.hpp>
#include <jack/jack.h>
#include <algorithm>

namespace {
	using namespace da;

	extern "C" int libda_jack_record_callback(jack_nframes_t frames, void* arg);
	extern "C" void libda_jack_record_shutdown(void* arg);
	class jack_record: public record::dev {
		settings m_s;
		jack_client_t* m_client;
		std::vector<jack_port_t*> m_ports;
		friend int libda_jack_record_callback(jack_nframes_t frames, void* arg);
		friend void libda_jack_record_shutdown(void* arg);
		int callback(jack_nframes_t frames) {
			std::vector<jack_default_audio_sample_t*> samples(m_ports.size());
			for (size_t ch = 0; ch < m_ports.size(); ++ch) samples[ch] = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(m_ports[ch], frames));
			std::vector<sample_t> buf;
			buf.reserve(frames * m_s.channels());
			for (jack_nframes_t fr = 0; fr < frames; ++fr) for (size_t ch = 0; ch < m_ports.size(); ++ch) buf.push_back(*samples[ch]++);
			pcm_data data(&buf[0], frames, m_ports.size());
			m_s.set_frames(frames);
			try {
				m_s.callback()(data, m_s);
			} catch (std::exception& e) {
				m_s.debug(std::string("Exception from recording callback: ") + e.what());
			}
			return 0;
		}
		void shutdown() { m_client = NULL; m_s.debug("da::jack_record: JACK server shutdown; processing terminated."); }
	  public:
		jack_record(settings& s): m_s(s) {
			m_s.set_subdev(m_s.subdev().empty() ? "libda_jack_record" : m_s.subdev());
			m_client = jack_client_new(m_s.subdev().c_str());
			if (!m_client) throw std::runtime_error("Unable to register JACK client (jackd not running or name " + m_s.subdev() + " already used?)");
			for (size_t i = 0; i < m_s.channels(); ++i) {
				std::string name = "capture_" + boost::lexical_cast<std::string>(i + 1);
				jack_port_t* p = jack_port_register(m_client, name.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
				if (!p) throw std::runtime_error("Unable to register JACK port");
				jack_connect(m_client, ("system:" + name).c_str(), (m_s.subdev() + ":" + name).c_str());
				m_ports.push_back(p);
			}
			m_s.set_rate(jack_get_sample_rate(m_client));
			m_s.set_frames(jack_get_buffer_size(m_client));
			jack_set_process_callback(m_client, libda_jack_record_callback, this);
			jack_on_shutdown(m_client, libda_jack_record_shutdown, this);
			jack_activate(m_client);
			connect();
			s = m_s;
		}
		~jack_record() { if (m_client) jack_client_close(m_client); }
		void connect() {
			for (size_t i = 0; i < m_ports.size(); ++i) {
				std::string name = jack_port_name(m_ports[i]);
				size_t pos = name.find(':');
				jack_connect(m_client, ("system" + name.substr(pos)).c_str(), name.c_str());
			}
		}
	};
	extern "C" int libda_jack_record_callback(jack_nframes_t frames, void* arg) { return static_cast<jack_record*>(arg)->callback(frames); }
	extern "C" void libda_jack_record_shutdown(void* arg) { static_cast<jack_record*>(arg)->shutdown(); }
	plugin::simple<record_plugin, jack_record> r(devinfo("jack", "JACK PCM input. JACK client name may be given as settings (default is \"libda_jack_record\")."));

	extern "C" int libda_jack_playback_callback(jack_nframes_t frames, void* arg);
	extern "C" void libda_jack_playback_shutdown(void* arg);
	class jack_playback: public playback::dev {
		settings m_s;
		jack_client_t* m_client;
		std::vector<jack_port_t*> m_ports;
		friend int libda_jack_playback_callback(jack_nframes_t frames, void* arg);
		friend void libda_jack_playback_shutdown(void* arg);
		void shutdown() { m_client = NULL; m_s.debug("da::jack_playback: JACK server shutdown; processing terminated."); }
		int callback(jack_nframes_t frames) {
			std::vector<sample_t> buf(frames * m_s.channels());
			pcm_data data(&buf[0], frames, m_ports.size());
			m_s.set_frames(frames);
			try {
				m_s.callback()(data, m_s);
			} catch (std::exception& e) {
				m_s.debug(std::string("Exception from playback callback: ") + e.what());
			}
			std::vector<jack_default_audio_sample_t*> samples(m_ports.size());
			for (size_t ch = 0; ch < m_ports.size(); ++ch) samples[ch] = static_cast<jack_default_audio_sample_t*>(jack_port_get_buffer(m_ports[ch], frames));
			for (jack_nframes_t fr = 0; fr < frames; ++fr) for (size_t ch = 0; ch < m_ports.size(); ++ch) *samples[ch]++ = buf[fr * m_ports.size() + ch];
			return 0;
		}
	  public:
		jack_playback(settings& s): m_s(s) {
			m_s.set_subdev(m_s.subdev().empty() ? "libda_jack_playback" : m_s.subdev());
			m_client = jack_client_new(m_s.subdev().c_str());
			if (!m_client) throw std::runtime_error("Unable to register JACK client (jackd not running or name " + m_s.subdev() + " already used?)");
			for (size_t i = 0; i < m_s.channels(); ++i) {
				std::string name = "playback_" + boost::lexical_cast<std::string>(i + 1);
				jack_port_t* p = jack_port_register(m_client, name.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
				if (!p) throw std::runtime_error("Unable to register JACK port");
				m_ports.push_back(p);
			}
			m_s.set_rate(jack_get_sample_rate(m_client));
			m_s.set_frames(jack_get_buffer_size(m_client));
			jack_set_process_callback(m_client, libda_jack_playback_callback, this);
			jack_on_shutdown(m_client, libda_jack_playback_shutdown, this);
			jack_activate(m_client);
			connect();
			s = m_s;
		}
		~jack_playback() { if (m_client) jack_client_close(m_client); }
		void connect() {
			for (size_t i = 0; i < m_ports.size(); ++i) {
				std::string name = jack_port_name(m_ports[i]);
				size_t pos = name.find(':');
				jack_connect(m_client, name.c_str(), ("system" + name.substr(pos)).c_str());
			}
		}
	};
	extern "C" int libda_jack_playback_callback(jack_nframes_t frames, void* arg) { return static_cast<jack_playback*>(arg)->callback(frames); }
	extern "C" void libda_jack_playback_shutdown(void* arg) { static_cast<jack_playback*>(arg)->shutdown(); }
	plugin::simple<playback_plugin, jack_playback> p(devinfo("jack", "JACK PCM output. JACK client name may be given as settings (default is \"libda_jack_playback\")."));
}


