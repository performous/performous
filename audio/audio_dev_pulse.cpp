#include "audio_dev.hpp"

#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <pulse/simple.h>
#include <pulse/error.h>

#include <iostream>

namespace {
	using namespace da;
	
	class pulse_record: public record::dev {
		settings m_s;
		pa_simple* m_stream;
		volatile bool m_quit;
		boost::scoped_ptr<boost::thread> m_thread;
	  public:
		pulse_record(settings& s):
		  m_s(s),
		  m_quit(false)
		{
			m_s.set_subdev(m_s.subdev().empty() ? "libda_pulse_record" : m_s.subdev());
			pa_sample_spec ss;
			{
				int val = 1; bool little_endian = *reinterpret_cast<char*>(&val);
				ss.format = little_endian ? PA_SAMPLE_FLOAT32LE : PA_SAMPLE_FLOAT32BE;
				ss.rate = m_s.rate();
				ss.channels = m_s.channels();
			}
			m_stream = pa_simple_new(NULL, m_s.subdev().c_str(), PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, NULL);
			if (!m_stream) throw std::runtime_error("PulseAudio pa_simple_new returned NULL.");
			if (m_s.frames() == settings::low) m_s.set_frames(256);
			else if (m_s.frames() == settings::high) m_s.set_frames(16384);
			m_thread.reset(new boost::thread(boost::ref(*this)));
			s = m_s;
		}
		~pulse_record() {
			m_quit = true;
			m_thread->join();
			pa_simple_free(m_stream);
		}
		void operator()() {
			std::vector<sample_t> buf;
			while (!m_quit) {
				const std::size_t channels = m_s.channels();
				buf.resize(m_s.frames() * channels);
				if (pa_simple_read(m_stream, &buf[0], buf.size() * sizeof(sample_t), NULL) < 0) continue;
				pcm_data data(&buf[0], buf.size() / channels, channels);
				try {
					m_s.callback()(data, m_s);
				} catch (std::exception& e) {
					m_s.debug(std::string("Exception from recording callback: ") + e.what());
				}
			}
		}
	};
	boost::plugin::simple<record_plugin, pulse_record> r(devinfo("pulse", "PulseAudio capture. Client (application) name can be given as settings."));

	class pulse_playback: public playback::dev {
		settings m_s;
		pa_simple* m_stream;
		volatile bool m_quit;
		boost::scoped_ptr<boost::thread> m_thread;
	  public:
		pulse_playback(settings& s):
		  m_s(s),
		  m_quit(false)
		{
			m_s.set_subdev(m_s.subdev().empty() ? "libda_pulse_playback" : m_s.subdev());
			pa_sample_spec ss;
			{
				int val = 1; bool little_endian = *reinterpret_cast<char*>(&val);
				ss.format = little_endian ? PA_SAMPLE_FLOAT32LE : PA_SAMPLE_FLOAT32BE;
				ss.rate = m_s.rate();
				ss.channels = m_s.channels();
			}
			m_stream = pa_simple_new(NULL, m_s.subdev().c_str(), PA_STREAM_PLAYBACK, NULL, "record", &ss, NULL, NULL, NULL);
			if (!m_stream) throw std::runtime_error("PulseAudio pa_simple_new returned NULL.");
			if (m_s.frames() == settings::low) m_s.set_frames(256);
			else if (m_s.frames() == settings::high) m_s.set_frames(16384);
			m_thread.reset(new boost::thread(boost::ref(*this)));
			s = m_s;
		}
		~pulse_playback() {
			m_quit = true;
			m_thread->join();
			pa_simple_free(m_stream);
		}
		void operator()() {
			std::vector<sample_t> buf;
			while (!m_quit) {
				const std::size_t channels = m_s.channels();
				buf.resize(m_s.frames() * channels);
				pcm_data data(&buf[0], buf.size() / channels, channels);
				try {
						m_s.callback()(data, m_s);
				} catch (std::exception& e) {
					m_s.debug(std::string("Exception from recording callback: ") + e.what());
				}
				int e;
				if (pa_simple_write(m_stream, &buf[0], buf.size() * sizeof(sample_t), &e) < 0) m_s.debug(pa_strerror(e));
			}
		}
	};
	boost::plugin::simple<playback_plugin, pulse_playback> p(devinfo("pulse", "PulseAudio playback. Client (application) name can be given as settings."));
}


