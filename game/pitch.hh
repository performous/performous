#pragma once

#include <complex>
#include <vector>
#include <list>
#include <algorithm>
#include <cmath>

/// struct to represent tones
struct Tone {
	static const std::size_t MAXHARM = 48; ///< maximum harmonics
	static const std::size_t MINAGE = 2; ///< minimum age
	double freq; ///< frequency
	double db; ///< dezibels
	double stabledb; ///< stable decibels
	double harmonics[MAXHARM]; ///< harmonics array
	std::size_t age; ///< age
	Tone(); 
	void print() const; ///< prints Tone
	bool operator==(double f) const; ///< equality operator
	void update(Tone const& t); ///< update Tone
	/// compares left and right volume
	static bool dbCompare(Tone const& l, Tone const& r) { return l.db < r.db; }
};

static inline bool operator==(Tone const& lhs, Tone const& rhs) { return lhs == rhs.freq; }
static inline bool operator!=(Tone const& lhs, Tone const& rhs) { return !(lhs == rhs); }
static inline bool operator<=(Tone const& lhs, Tone const& rhs) { return lhs.freq < rhs.freq || lhs == rhs; }
static inline bool operator>=(Tone const& lhs, Tone const& rhs) { return lhs.freq > rhs.freq || lhs == rhs; }
static inline bool operator<(Tone const& lhs, Tone const& rhs) { return lhs.freq < rhs.freq && lhs != rhs; }
static inline bool operator>(Tone const& lhs, Tone const& rhs) { return lhs.freq > rhs.freq && lhs != rhs; }

static const unsigned FFT_P = 10;
static const std::size_t FFT_N = 1 << FFT_P;
static const std::size_t BUF_N = 2 * FFT_N;

/// analyzer class
 /** class to analyze input audio and transform it into useable data
 */
class Analyzer {
  public:
	/// fast fourier transform vector
	typedef std::vector<std::complex<float> > fft_t;
	/// list of tones
	typedef std::list<Tone> tones_t;
	/// constructor
	Analyzer(double rate, std::size_t step = 200);
	/** Add input data to buffer. This is thread-safe (against other functions). **/
	template <typename InIt> void input(InIt begin, InIt end) {
		while (begin != end) {
			float s = *begin;
			++begin;
			m_peak *= 0.999;
			float p = s * s;
			if (p > m_peak) m_peak = p;
			size_t w = m_bufWrite;
			size_t w2 = (m_bufWrite + 1) % BUF_N;
			size_t r = m_bufRead;
			if (w2 == r) m_bufRead = (r + 1) % BUF_N;
			m_buf[w] = s;
			m_bufWrite = w2;
		}
	}
	/** Call this to process all data input so far. **/
	void process();
	/** Get the raw FFT. **/
	fft_t const& getFFT() const { return m_fft; }
	/** Get the peak level in dB (negative value, 0.0 = clipping). **/
	double getPeak() const { return 10.0 * log10(m_peak); }
	/** Get a list of all tones detected. **/
	tones_t const& getTones() const { return m_tones; }
	/** Find a tone within the singing range; prefers strong tones around 200-400 Hz. **/
	Tone const* findTone(double minfreq = 70.0, double maxfreq = 700.0) const {
		if (m_tones.empty()) { m_oldfreq = 0.0; return NULL; }
		double db = std::max_element(m_tones.begin(), m_tones.end(), Tone::dbCompare)->db;
		Tone const* best = NULL;
		double bestscore = 0;
		for (tones_t::const_iterator it = m_tones.begin(); it != m_tones.end(); ++it) {
			if (it->db < db - 20.0 || it->freq < minfreq || it->age < Tone::MINAGE) continue;
			if (it->freq > maxfreq) break;
			double score = it->db - std::max(180.0, std::abs(it->freq - 300.0)) / 10.0;
			if (m_oldfreq != 0.0 && std::fabs(it->freq/m_oldfreq - 1.0) < 0.05) score += 10.0;
			if (best && bestscore > score) break;
			best = &*it;
			bestscore = score;
		}
		m_oldfreq = (best ? best->freq : 0.0);
		return best;
	}

  private:
	std::size_t m_step;
	double m_rate;
	std::vector<float> m_window;
	float m_buf[2 * BUF_N];
	volatile size_t m_bufRead, m_bufWrite;
	fft_t m_fft;
	std::vector<float> m_fftLastPhase;
	double m_peak;
	tones_t m_tones;
	mutable double m_oldfreq;
	bool calcFFT();
	void calcTones();
	void mergeWithOld(tones_t& tones) const;
};
