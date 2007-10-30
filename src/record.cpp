#include <record.h>

#include <fft.hpp>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

Tone::Tone():
  m_freqSum(0.0),
  m_harmonics(0),
  m_hEven(0),
  m_hHighest(0),
  m_dbHighest(-std::numeric_limits<double>::infinity()),
  m_dbHighestH(0)
{}

void Tone::print() const {
	std::cout << freq() << " Hz, " << m_harmonics << " harmonics (" << m_hEven << " are even), strongest is x" << m_dbHighestH << " @ " << m_dbHighest << " dB, highest is x" << m_hHighest << std::endl;
}

void Tone::combine(Peak& p, unsigned int h) {
	m_freqSum += p.freq() / h;
	++m_harmonics;
	if (h % 2 == 0) ++m_hEven;
	if (h > m_hHighest) m_hHighest = h;
	if (p.db() > m_dbHighest) {
		m_dbHighest = p.db();
		m_dbHighestH = h;
	}
}

bool Tone::isWeak() const {
	if (m_harmonics > 2) return db() < -45.0;
	if (m_harmonics > 1) return db() < -35.0;
	return db() < -25.0;
}

bool Tone::operator==(double f) const {
	return fabs(freq() / f - 1.0) < 0.03;
}

Tone& Tone::operator+=(Tone const& t) {
	m_freqSum += t.m_freqSum;
	m_harmonics += t.m_harmonics;
	m_hEven += t.m_hEven;
	if (t.m_hHighest > m_hHighest) m_hHighest = t.m_hHighest;
	if (t.m_dbHighest > m_dbHighest) {
		m_dbHighest = t.m_dbHighest;
		m_dbHighestH = t.m_dbHighestH;
	}
	return *this;
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

static int match(std::vector<Peak> const& peaks, int pos, double freq) {
	int best = pos;
	if (fabs(peaks[pos-1].freq() - freq) < fabs(peaks[best].freq() - freq)) best = pos - 1;
	if (fabs(peaks[pos+1].freq() - freq) < fabs(peaks[best].freq() - freq)) best = pos + 1;
	return best;
}

namespace {
	bool sqrLT(float a, float b) { return a * a < b * b; }
}

void Analyzer::operator()(da::pcm_data& indata, da::settings const& s)
{
	// Precalculated constants
	const double freqPerBin = double(s.rate()) / FFT_N;
	const double phaseStep = 2.0 * M_PI * m_step / FFT_N;
	const double normCoeff = 1.0 / FFT_N; // WTF? This was: 4.0 / ((double)FFT_N * FFT_N);
	const double minMagnitude = pow(10, -60.0 / 10.0) / normCoeff;

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
		// Process only up to 3000 Hz
		size_t kMax = std::min(FFT_N / 2, (size_t)(3000.0 / freqPerBin));
		std::vector<Peak> peaks(kMax + 1);

		for (size_t k = 0; k <= kMax; ++k) {
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

			if (magnitude > minMagnitude) {
				peaks[k].freq(freq);
				peaks[k].db(10.0 * log10(normCoeff * magnitude));
			}
		}
		// Find the tones (collections of harmonics) from the array of peaks
		// TODO: proper handling of tones with "missing fundamental" (is this needed?)
		std::vector<Tone> tones;
		double db = -std::numeric_limits<double>::infinity();
		for (size_t k = 2; k < kMax; ++k) {
			// Prefilter out too low freqs, too silent bins and bins that are weaker than their neighbors
			if (peaks[k].freq() < 80.0 || peaks[k].freq() > 700.0 || peaks[k].db() < -30.0 || peaks[k].db() < peaks[k-1].db() || peaks[k].db() < peaks[k+1].db()) continue;
			// Find the base peak (fundamental frequency)
			int harmonic = 1;
			int misses = 0;
			for (int h = 2; k / h > 2; ++h) {
				double freq = peaks[k].freq() / h;
				if (freq < 40.0 || ++misses > 3) break;
				int best = match(peaks, k / h, freq);
				if (peaks[best].db() < -30.0 || fabs(peaks[best].freq() / freq - 1.0) > .03) continue;
				misses = 0;
				harmonic = h;
			}
			std::vector<Tone>::iterator it = std::find(tones.begin(), tones.end(), peaks[k].freq() / harmonic);
			if (it == tones.end()) {
				tones.push_back(Tone());
				it = tones.end() - 1;
			}
			it->combine(peaks[k], harmonic);
			db = std::max(db, it->db());
		}
		// Remove weak tones
		tones.erase(std::remove_if(tones.begin(), tones.end(), std::mem_fun_ref(&Tone::isWeak)), tones.end());

		/* Debug printout
		if (!tones.empty()) {
			std::cerr << "\x1B[2J\x1B[1;1H" << tones.size() << " tones\n" << std::fixed << std::setprecision(1);
			std::for_each(tones.begin(), tones.end(), std::mem_fun_ref(&Tone::print));
		}
		*/

		// The following block controls the tones list output.
		// - A tone is only enabled if it is found in old (m_oldTones) and current (tones) list.
		// - A tone is disabled if it is not found in either of these lists
		// In addition, a tone is always updated with the latest information, if it is found in
		// either of these two internal lists.
		{
			std::vector<Tone> is, un, tmp;
			// Calculate intersection: only those that are in both the old and the current tones (use the current one)
			std::set_intersection(tones.begin(), tones.end(), m_oldTones.begin(), m_oldTones.end(), std::back_inserter(is));
			// Calculate union: all those that are either tones (use the most recent one if both are available)
			std::set_union(tones.begin(), tones.end(), m_oldTones.begin(), m_oldTones.end(), std::back_inserter(un));
			// Take from m_tones only those that have played recently, into tmp (use the one from union)
			std::set_intersection(un.begin(), un.end(), m_tones.begin(), m_tones.end(), std::back_inserter(tmp));
			boost::mutex::scoped_lock l(m_mutex); // Critical section: writing to m_tones
			m_tones.clear();
			// Combine tmp with the stable new tones (the intersection), into m_tones.
			std::set_union(tmp.begin(), tmp.end(), is.begin(), is.end(), back_inserter(m_tones));
		}

		// Find the singing frequency
		double freq = 0.0;
		for (size_t i = 0; i < m_tones.size(); ++i) {
			if (m_tones[i].db() < db - 4.0) continue;
			freq = m_tones[i].freq();
		}
		m_freq = freq; // Critical: atomic modification

		m_oldTones = tones;
	}
}

std::string MusicalScale::getNoteStr(double freq) const {
	int id = getNoteId(freq);
	if (id == -1) return std::string();
	static const char * note[12] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
	std::ostringstream oss;
	// Acoustical Society of America Octave Designation System
	//int octave = 2 + id / 12;
	oss << note[id%12] << " " << (int)round(freq) << " Hz";
	return oss.str();
}

unsigned int MusicalScale::getNoteNum(int id) const {
	switch (id % 12) {
	  case 0: return 0;
	  case 1: return 0;
	  case 2: return 1;
	  case 3: return 1;
	  case 4: return 2;
	  case 5: return 3;
	  case 6: return 3;
	  case 7: return 4;
	  case 8: return 4;
	  case 9: return 5;
	  case 10: return 5;
	  default: return 6;
	}
}

bool MusicalScale::isSharp(int id) const {
	if (id < 0) throw std::logic_error("MusicalScale::isSharp: Invalid note ID");
	id %= 12;
	switch (id) {
	  case 1: case 3: case 6: case 8: case 10: return true;
	}
	return false;
}

double MusicalScale::getNoteFreq(int id) const
{
	if (id == -1) return 0.0;
	return m_baseFreq * pow(2.0, (id - m_baseId) / 12.0);
}

int MusicalScale::getNoteId(double freq) const
{
	if (freq < 1.0) return -1;
	int id = (int) (m_baseId + 12.0 * log(freq / m_baseFreq) / log(2) + 0.5);
	return id < 0 ? -1 : id;
}

double MusicalScale::getNoteOffset(double freq) const {
	double frac = freq / getNoteFreq(getNoteId(freq));
	return 12.0 * log(frac) / log(2);
}

