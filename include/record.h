#ifndef __RECORD_H_
#define __RECORD_H_

#include <boost/thread/mutex.hpp>
#include <audio.hpp>
#include <cstddef>
#include <deque>
#include <iostream>
#include <limits>
#include <list>
#include <vector>

struct Tone {
	static const std::size_t MAXHARM = 48;
	static const std::size_t MINAGE = 2;
	double freq;
	double db;
	double stabledb;
	double harmonics[MAXHARM];
	std::size_t age;
	Tone();
	void print() const;
	bool operator==(double f) const;
	void update(Tone const& t);
};

static inline bool operator==(Tone const& lhs, Tone const& rhs) { return lhs == rhs.freq; }
static inline bool operator!=(Tone const& lhs, Tone const& rhs) { return !(lhs == rhs); }
static inline bool operator<=(Tone const& lhs, Tone const& rhs) { return lhs.freq < rhs.freq || lhs == rhs; }
static inline bool operator>=(Tone const& lhs, Tone const& rhs) { return lhs.freq > rhs.freq || lhs == rhs; }
static inline bool operator<(Tone const& lhs, Tone const& rhs) { return lhs.freq < rhs.freq && lhs != rhs; }
static inline bool operator>(Tone const& lhs, Tone const& rhs) { return lhs.freq > rhs.freq && lhs != rhs; }

class Analyzer {
  public:
	typedef std::list<Tone> tones_t;
	Analyzer(std::size_t step = 500);
	void operator()(da::pcm_data& data, da::settings const& s);
	/** Get the peak level in dB (negative value, 0.0 = clipping). **/
	double getPeak() const { return m_peak; }
	/** Get the primary (singing) frequency. **/
	double getFreq() const { return m_freq; }
	/** Get a list of all tones detected. **/
	tones_t getTones() const {
		boost::mutex::scoped_lock l(m_mutex);
		return m_tones;
	}
  private:
	mutable boost::mutex m_mutex;
	std::size_t m_step;
	std::vector<float> m_fftLastPhase;
	std::vector<float> m_window;
	volatile double m_peak;
	volatile double m_freq;
	std::deque<float> m_buf; // Sample buffer
	tones_t m_tones; // Synchronized access only!
};

class Capture {
	static const std::size_t DEFAULT_RATE = 48000;
	Analyzer m_analyzer;
	da::settings m_rs;
	da::record m_record;
  public:
	Capture(std::string const& device = "", std::size_t rate = DEFAULT_RATE):
	  m_rs(da::settings(device)
	  .set_callback(boost::ref(m_analyzer))
	  .set_channels(1)
	  .set_rate(rate)
	  .set_debug(std::cerr)),
	  m_record(m_rs)
	{}
	~Capture() {}
	Analyzer const& analyzer() const { return m_analyzer; }
};

#endif
