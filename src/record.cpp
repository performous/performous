#include <record.h>

#include <fft.hpp>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <fstream>

static const unsigned FFT_P = 11;
static const std::size_t FFT_N = 1 << FFT_P;

// Limit the range to avoid noise and useless computation
static const double FFT_MINFREQ = 50.0;
static const double FFT_MAXFREQ = 3000.0;

Tone::Tone():
  freq(0.0),
  db(-std::numeric_limits<double>::infinity()),
  stabledb(-std::numeric_limits<double>::infinity()),
  age()
{
	for (std::size_t i = 0; i < MAXHARM; ++i)
	  harmonics[i] = -std::numeric_limits<double>::infinity();
}

void Tone::print() const {
	if (age < Tone::MINAGE) return;
	std::cout << std::fixed << std::setprecision(1) << freq << " Hz, age " << age << ", " << db << " dB:";
	for (std::size_t i = 0; i < 8; ++i) std::cout << " " << harmonics[i];
	std::cout << std::endl;
}

bool Tone::operator==(double f) const {
	return std::abs(freq / f - 1.0) < 0.05;
}

Analyzer::Analyzer(size_t step):
  m_step(step),
  m_fftLastPhase(FFT_N / 2),
  m_window(FFT_N),
  m_peak(-std::numeric_limits<double>::infinity()),
  m_freq(0.0)
{
  	// Hamming window
	for (size_t i=0; i < FFT_N; i++) {
		m_window[i] = 0.53836 - 0.46164 * std::cos(2.0 * M_PI * i / FFT_N - 1);
	}
}

namespace {
	bool sqrLT(float a, float b) { return a * a < b * b; }

	struct Peak {
		double freq;
		double db;
		bool harm[Tone::MAXHARM];
		Peak(double _freq = 0.0, double _db = -std::numeric_limits<double>::infinity()):
		  freq(_freq), db(_db)
		{
			for (std::size_t i = 0; i < Tone::MAXHARM; ++i) harm[i] = false;
		}
		void clear() {
			freq = 0.0;
			db = -std::numeric_limits<double>::infinity();
		}
	};

	static Peak& match(std::vector<Peak>& peaks, std::size_t pos) {
		std::size_t best = pos;
		if (peaks[pos - 1].db > peaks[best].db) best = pos - 1;
		if (peaks[pos + 1].db > peaks[best].db) best = pos + 1;
		return peaks[best];
	}

}

void Analyzer::operator()(da::pcm_data& indata, da::settings const& s)
{
	// Precalculated constants
	const double freqPerBin = double(s.rate()) / FFT_N;
	const double phaseStep = 2.0 * M_PI * m_step / FFT_N;
	const double normCoeff = 1.0 / FFT_N;
	const double minMagnitude = pow(10, -100.0 / 20.0) / normCoeff; // -100 dB

	std::copy(indata.begin(0), indata.end(0), std::back_inserter(m_buf));

	while (m_buf.size() >= FFT_N) {
		// Calculate peak
		{
			double tmp = *std::max_element(m_buf.begin(), m_buf.begin() + FFT_N, sqrLT);
			m_peak = 10.0 * log10(tmp * tmp); // Critical: atomic write
		}
		// Calculate FFT
		std::vector<std::complex<float> > data = da::fft<FFT_P>(m_buf.begin(), m_window);
		// Erase one step of samples
		m_buf.erase(m_buf.begin(), m_buf.begin() + m_step);
		// Limit frequency range of processing
		size_t kMin = std::max(size_t(1), size_t(FFT_MINFREQ / freqPerBin));
		size_t kMax = std::min(FFT_N / 2, size_t(FFT_MAXFREQ / freqPerBin));
		std::vector<Peak> peaks(kMax + 1); // One extra to simplify loops
		for (size_t k = 1; k <= kMax; ++k) {
			double magnitude = std::abs(data[k]);
			double phase = std::arg(data[k]);

			// process phase difference
			double delta = phase - m_fftLastPhase[k];
			m_fftLastPhase[k] = phase;
			// subtract expected phase difference
			delta -= k * phaseStep;
			// map delta phase into +/- M_PI interval
			delta = remainder(delta, 2.0 * M_PI);
			// calculate diff from bin center frequency
			delta /= phaseStep; // ((double)FFT_N / step) / (2.0 * M_PI);
			// process the k-th partials' true frequency
			double freq = (k + delta) * freqPerBin;

			if (freq > 1.0 && magnitude > minMagnitude) {
				peaks[k].freq = freq;
				peaks[k].db = 20.0 * log10(normCoeff * magnitude);
			}
		}
		// Prefilter raw data
		double prevdb = peaks[0].db;
		for (size_t k = 1; k < kMax; ++k) {
			double db = peaks[k].db;
			if (db > prevdb) peaks[k - 1].clear();
			if (db < prevdb) peaks[k].clear();
			prevdb = db;
		}
		// Find the tones (collections of harmonics) from the array of peaks
		tones_t tones;
		double db = -std::numeric_limits<double>::infinity();
		for (size_t k = kMax - 1; k >= kMin; --k) {
			if (peaks[k].db < -70.0) continue;
			std::size_t bestDiv = 1;
			int bestScore = 0;
			for (std::size_t div = 2; div <= Tone::MAXHARM && k / div > 1; ++div) {
				double freq = peaks[k].freq / div; // Fundamental
				int score = 0;
				for (std::size_t n = 1; n < div && n < 8; ++n) {
					Peak& p = match(peaks, k * n / div);
					--score;
					if (p.db < -90.0 || std::abs(p.freq / n / freq - 1.0) > .03) continue;
					if (n == 1) score += 4; // Extra for fundamental
					score += 2;
				}
				if (score > bestScore) {
					bestScore = score;
					bestDiv = div;
				}
			}
			Tone t;
			std::size_t count = 0;
			double freq = peaks[k].freq / bestDiv;
			t.db = peaks[k].db;
			for (std::size_t n = 1; n <= bestDiv; ++n) {
				Peak& p = match(peaks, k * n / bestDiv);
				if (std::abs(p.freq / n / freq - 1.0) > .03) continue;
				if (p.db > t.db - 10.0) {
					t.db = std::max(t.db, p.db);
					++count;
					t.freq += p.freq / n;
				}
				t.harmonics[n - 1] = p.db;
				p.clear();
			}
			t.freq /= count;
			if (t.db > -50.0 - 3.0 * count) {
				db = std::max(db, t.db);
				t.stabledb = t.db;
				tones.push_back(t);
			}
		}
		tones.sort();
		// Merge old and new tones
		{
			tones_t::iterator it = tones.begin();
			for (tones_t::const_iterator oldit = m_tones.begin(); oldit != m_tones.end(); ++oldit) {
				while (it != tones.end() && *it < *oldit) ++it;
				if (it == tones.end() || *it != *oldit) {
					if (oldit->db > -80.0) {
						Tone& t = *tones.insert(it, *oldit);
						t.db -= 5.0;
						t.stabledb -= 0.1;
					}
				} else if (*it == *oldit) {
					it->age = oldit->age + 1;
					it->stabledb = 0.8 * oldit->stabledb + 0.2 * it->db;
					it->freq = 0.5 * oldit->freq + 0.5 * it->freq;
				}
			}
		}
		{
			boost::mutex::scoped_lock l(m_mutex); // Critical section: writing to m_tones
			m_tones.swap(tones);
		}

		// Find the singing frequency
		double freq = 0.0;
		for (tones_t::const_iterator it = m_tones.begin(); it != m_tones.end(); ++it) {
			if (it->db < db - 20.0 || it->freq < 70.0 || it->age < Tone::MINAGE) continue;
			if (it->freq > 600.0) break;
			freq = it->freq;
			break;
		}
		m_freq = freq; // Critical: atomic modification

		/*
		if (!m_tones.empty()) {
			std::cerr << "\x1B[2J\x1B[1;1H" << m_tones.size() << " tones\n" << std::fixed << std::setprecision(1);
			std::for_each(m_tones.begin(), m_tones.end(), std::mem_fun_ref(&Tone::print));
		}

		{
			static int m_frame = 0;
			++m_frame;
			std::ostringstream oss;
			oss << "dump-" << m_frame << ".dat";
			std::ofstream f(oss.str().c_str());
			f << std::setprecision(3) << "# Freqs:\n";
			for (tones_t::const_iterator it = m_tones.begin(); it != m_tones.end(); ++it) {
				f << "# " << it->freq() << " Hz, " << it->db() << " dB" << std::endl;
			}
			for (size_t k = 0; k <= kMax; ++k) {
				double magnitude = std::abs(data[k]);
				double phase = std::arg(data[k]);
				double fr = k * freqPerBin;
				f << fr << '\t' << magnitude << '\t' << phase << std::endl;
			}
		}
		*/
	}
}

