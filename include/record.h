#ifndef __RECORD_H_
#define __RECORD_H_

#include <boost/thread/mutex.hpp>
#include <audio.hpp>
#include <cstddef>
#include <deque>
#include <iostream>
#include <limits>
#include <vector>

struct Peak {
	double freq;
	double db;
	Peak(double freq = 0.0, double db = -std::numeric_limits<double>::infinity()): freq(freq), db(db) {}
};

class Tone {
	double m_freqSum; // Sum of the fundamental frequencies of all harmonics
	unsigned int m_harmonics;
	unsigned int m_hEven;
	unsigned int m_hHighest;
	double m_dbHighest;
	unsigned int m_dbHighestH;
  public:
	Tone();
	void print() const;
	void combine(Peak& p, unsigned int h);
	bool isWeak() const;
	double db() const { return m_dbHighest; }
	double freq() const { return m_freqSum / m_harmonics; }
	bool operator==(double f) const;
	Tone& operator+=(Tone const& t);
};

static inline bool operator==(Tone const& lhs, Tone const& rhs) { return lhs == rhs.freq(); }
static inline bool operator!=(Tone const& lhs, Tone const& rhs) { return !(lhs == rhs); }
static inline bool operator<=(Tone const& lhs, Tone const& rhs) { return lhs.freq() < rhs.freq() || lhs == rhs; }
static inline bool operator>=(Tone const& lhs, Tone const& rhs) { return lhs.freq() > rhs.freq() || lhs == rhs; }
static inline bool operator<(Tone const& lhs, Tone const& rhs) { return lhs.freq() < rhs.freq() && lhs != rhs; }
static inline bool operator>(Tone const& lhs, Tone const& rhs) { return lhs.freq() > rhs.freq() && lhs != rhs; }

class Analyzer {
	static const unsigned FFT_P = 12;
	static const std::size_t FFT_N = 1 << FFT_P;
  public:
	Analyzer(std::size_t step = 1500);
	void operator()(da::pcm_data& data, da::settings const& s);
	/** Get the peak level in dB (negative value, 0.0 = clipping). **/
	double getPeak() const { return m_peak; }
	/** Get the primary (singing) frequency. **/
	double getFreq() const { return m_freq; }
	/** Get a list of all tones detected. **/
	std::vector<Tone> getTones() const {
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
	std::vector<Tone> m_tones; // Synchronized access only!
	std::vector<Tone> m_oldTones;
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

class MusicalScale {
	double baseFreq;
	static const int baseId = 33;
  public:
	MusicalScale(double baseFreq = 440.0): baseFreq(baseFreq) {}
	std::string getNoteStr(double freq) const;
	double getNoteFreq(int id) const;
	int getNoteId(double freq) const;
	double getNoteOffset(double freq) const;
};

#endif
