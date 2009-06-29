#include <libda/audio_dev.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

namespace {

	using namespace da;
	// Boost WTF time format, directly from C...
	boost::xtime& operator+=(boost::xtime& time, double seconds) {
		double nsec = 1e9 * (time.sec + seconds) + time.nsec;
		time.sec = boost::xtime::xtime_sec_t(nsec / 1e9);
		time.nsec = boost::xtime::xtime_nsec_t(std::fmod(nsec, 1e9));
		return time;
	}

	class tonegen: public record::dev {
		class sin_generator {
			double m_phase, m_step;
			sample_t m_amplitude;
		  public:
			sin_generator(double freq, double rate, sample_t amplitude, double phase):
			  m_phase(2.0 * M_PI * phase), m_step(2.0 * M_PI * freq / rate), m_amplitude(amplitude) {}
			sample_t operator()() {
				double s = m_amplitude * std::sin(m_phase = fmod(m_phase + m_step, 2.0 * M_PI));
				return return static_cast<sample_t>(s);
			}
			operator sample_t() { return (*this)(); }
		};
		class accumgen {
			std::vector<sin_generator>& m_gen;
			std::size_t m_channels;
			std::size_t m_ch;
			double m_val;
		  public:
			accumgen(std::vector<sin_generator>& gen, std::size_t channels): m_gen(gen), m_channels(channels), m_ch(), m_val() {}
			sample_t operator()() {
				if (m_ch % m_channels == 0) m_val = std::accumulate(m_gen.begin(), m_gen.end(), sample_t());
				m_ch = (m_ch + 1) % m_channels;
				return static_cast<sample_t>(m_val);
			}
		};
		std::vector<sin_generator> gen;
		settings s;
		volatile bool quit;
		boost::scoped_ptr<boost::thread> thread;
		boost::xtime time;
		void add(std::string const& tonestr) {
			double freq = 440.0, amplitude = 0.1, phase = 0.0;
			using namespace boost::spirit::classic;
			using namespace boost::lambda;
			placeholder1_type arg1;
			if (!parse(tonestr.c_str(),
			  !(limit_d(1.0, s.rate() / 2.0)[real_p][assign_a(freq)]) >> *(!ch_p('.') >> (
			  str_p("amplitude(") >> (max_limit_d(0.0)[real_p][var(amplitude) = bind(static_cast<double(*)(double, double)>(std::pow), 10.0, arg1 / 20.0)] | real_p[assign_a(amplitude)]) |
			  str_p("phase(") >> limit_d(0.0, 1.0)[real_p][assign_a(phase)]
			  ) >> ')')
			  ).full)
			  throw std::invalid_argument("Invalid parameters for tonegen. See help for more information.");
			{
				std::ostringstream oss;
				oss << "  " << freq << ".amplitude(" << amplitude << ").phase(" << phase << ")";
				s.debug(oss.str());
			}
			gen.push_back(sin_generator(freq, s.rate(), static_cast<sample_t>(amplitude), phase));
		}
	  public:
		tonegen(settings& s_orig): s(s_orig), quit(false) {
			if (s.frames() == settings::low) s.set_frames(256);
			if (s.frames() == settings::high) s.set_frames(16384);
			std::istringstream iss(s.subdev());
			s.debug("Tone generator:");
			for (std::string tmp; std::getline(iss, tmp, ':') || gen.empty();) add(tmp);
			boost::xtime_get(&time, boost::TIME_UTC); // Get current time
			thread.reset(new boost::thread(boost::ref(*this)));
			s_orig = s;
		}
		~tonegen() {
			quit = true;
			thread->join();
		}
		void operator()(void) {
			while (!quit) {
				boost::thread::sleep(time += double(s.frames())/s.rate());
				try {
					std::vector<sample_t> buf(s.frames() * s.channels());
					std::generate(buf.begin(), buf.end(), accumgen(gen, s.channels()));
					pcm_data data(&buf[0], s.frames(), s.channels());
					s.callback()(data, s);
				} catch (std::exception& e) {
					s.debug(std::string("Exception from recording callback: ") + e.what());
				}
			}
		}
	};
	plugin::simple<record_plugin, tonegen> r(devinfo("~tone", "Tone generator. Settings format: tone1:tone2:..., where each tone is specified as frequency in Hz, optionally followed by amplitude or phase settings, e.g. 440.amplitude(-20).phase(0.25). Amplitude values <= 0 are taken as decibels and > 0 are taken as absolute values. If no parameters are given, a 440 Hz tone will be generated."));
}

