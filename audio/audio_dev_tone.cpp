#include "audio_dev.hpp"
#include <boost/scoped_ptr.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/spirit/core.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <signal.h>
#include <sstream>

namespace da {

	namespace {
		// Boost WTF time format, directly from C...
		boost::xtime& operator+=(boost::xtime& time, double seconds) {
			double nsec = 1e9 * (time.sec + seconds) + time.nsec;
			time.sec = boost::xtime::xtime_sec_t(nsec / 1e9);
			time.nsec = boost::xtime::xtime_nsec_t(std::fmod(nsec, 1e9));
			return time;
		}
	}

	class tonegen: public record::dev {
		class sin_generator {
			double phase, step;
			sample_t amplitude;
		  public:
			sin_generator(double freq, double rate, double amplitude, double phase):
			  phase(2.0 * M_PI * phase), step(2.0 * M_PI * freq / rate), amplitude(amplitude) {}
			sample_t operator()() { return amplitude * std::sin(phase = fmod(phase + step, 2.0 * M_PI)); }
			operator sample_t() { return (*this)(); }
		};
		class accumgen {
			std::vector<sin_generator>& gen;
			std::size_t channels;
			std::size_t ch;
			double val;
		  public:
			accumgen(std::vector<sin_generator>& gen, std::size_t channels): gen(gen), channels(channels), ch(), val() {}
			sample_t operator()() {
				if (ch % channels == 0) val = std::accumulate(gen.begin(), gen.end(), sample_t());
				ch = (ch + 1) % channels;
				return val;
			}
		};
		static reg_dev reg;
		static dev* create(settings& s) { return new tonegen(s); }
		std::vector<sin_generator> gen;
		settings s;
		volatile bool quit;
		boost::scoped_ptr<boost::thread> thread;
		boost::xtime time;
		void add(std::string const& tonestr) {
			double freq = 440.0, amplitude = 0.3, phase = 0.0;
			using namespace boost::spirit;
			using namespace boost::lambda;
			if (!parse(tonestr.c_str(),
			  !(limit_d(1.0, s.rate / 2.0)[real_p][assign_a(freq)]) >> *(!ch_p('.') >> (
			  str_p("amplitude(") >> (max_limit_d(0.0)[real_p][var(amplitude) = bind(static_cast<double(*)(double, double)>(std::pow), 10.0, _1 / 20.0)] | real_p[assign_a(amplitude)]) |
			  str_p("phase(") >> limit_d(0.0, 1.0)[real_p][assign_a(phase)]
			  ) >> ')')
			  ).full)
			  throw std::invalid_argument("Invalid parameters for tonegen. See help for more information.");
			if (s.debug) *s.debug << "  " << freq << ".amplitude(" << amplitude << ").phase(" << phase << ")" << std::endl;
			gen.push_back(sin_generator(freq, s.rate, amplitude, phase));
		}
	  public:
		tonegen(settings& s_orig): s(s_orig), quit(false) {
			if (s.frames == settings::low) s.frames = 256;
			if (s.frames == settings::high) s.frames = 16384;
			std::istringstream iss(s.subdev);
			if (s.debug) *s.debug << "Tone generator:" << std::endl;
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
				boost::thread::sleep(time += double(s.frames)/s.rate);
				try {
					std::vector<sample_t> buf(s.frames * s.channels);
					std::generate(buf.begin(), buf.end(), accumgen(gen, s.channels));
					pcm_data data(&buf[0], s.frames, s.channels);
					s.callback(data, s);
				} catch (std::exception& e) {
					if (s.debug) *s.debug << "Exception from recording callback: " << e.what() << std::endl;
				}
			}
		}
	};

	tonegen::reg_dev tonegen::reg("~tone", tonegen::create);
	
}

