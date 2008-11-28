#include "audio.hh"

#include "util.hh"
#include "xtime.hh"
#include <functional>
#include <iostream>
#include <cmath>
#include "screen.hh"

Audio::Audio(std::string const& pdev, unsigned int rate):
	m_crossfade(0),
	m_crossbuf(0.4 * 48000 * 2), // 0.4 seconds at 48 kHz, 2 ch
	m_volume(1.0),
	m_volumeMusic(100),
	m_volumePreview(70),
	m_paused(false),
	m_prebuffering(false),
	m_rs(da::settings(pdev)
	.set_callback(boost::ref(*this))
	.set_channels(2)
	.set_rate(rate)
	.set_debug(std::cerr)),
	m_playback(m_rs)
{}

void Audio::operator()(da::pcm_data& areas, da::settings const&) {
	std::size_t samples = areas.channels * areas.frames;
	unsigned int size = 0;
	static double phase = 0.0;
	Notes const* n = m_notes;
	double volume = m_volume * (n ? 0.3 : 1.0);
	if (m_mpeg && !m_paused) {
		if (m_prebuffering && m_mpeg->audioQueue.percentage() > 0.9) m_prebuffering = false;
		if (!m_prebuffering) {
			std::vector<int16_t> buf;
			m_mpeg->audioQueue.tryPop(buf, samples);
			std::transform(buf.begin(), buf.end(), areas.m_buf, da::conv_from_s16);
			size = buf.size();
			if (size < samples && !m_mpeg->audioQueue.eof() && m_mpeg->position() > 1.0) std::cerr << "Warning: audio decoding too slow (buffer underrun): " << std::endl;
			if (volume != 1.0) std::transform(areas.m_buf, areas.m_buf + size, areas.m_buf, std::bind1st(std::multiplies<double>(), volume));
		}
	}
	std::fill(areas.m_buf + size, areas.m_buf + samples, 0.0f);
	// Synthesize tones
	if (n && m_mpeg && !m_paused) {
		double t = m_mpeg->position();
		Notes::const_iterator it = n->begin();
		while (it != n->end() && it->end < t) ++it;
		if (it == n->end() || it->type == Note::SLEEP || it->begin > t) { phase = 0.0; return; }
		double freq = MusicalScale().getNoteFreq(it->note % 12 + 12);
		double value = 0.0;
		for (size_t i = 0; i < samples; ++i) {
			if (i % areas.channels == 0) {
				value = 0.2 * std::sin(phase) + 0.2 * std::sin(2 * phase) + 0.2 * std::sin(4 * phase);
				phase += 2.0 * M_PI * freq / m_rs.rate();
			}
			areas.m_buf[i] += value;
		}
	}
	// Need to crossfade?
	if (m_crossfade < m_crossbuf.size()) {
		size_t cf = m_crossfade;
		double f = 0.0;
		for (size_t i = 0; i < samples && cf < m_crossbuf.size(); ++i) {
			if (i % areas.channels == 0) f = double(cf)/m_crossbuf.size();
			areas.m_buf[i] = f * areas.m_buf[i] + (1.0 - f) * m_crossbuf[cf++];
		}
		m_crossfade = cf;
	}
}

void Audio::setVolume_internal(unsigned int volume) {
	if (volume == 0) { m_volume = 0.0; return; }
	m_volume = std::pow(10.0, (volume - 100.0) / 100.0 * 2.0);
}

void Audio::playMusic(std::string const& filename) {
	stopMusic();
	try {
		m_mpeg.reset(new CFfmpeg(false, true, filename, m_rs.rate()));
		if (m_mpeg->duration() < 0) return;
		m_length = m_mpeg->duration();
		setVolume_internal(m_volumeMusic);
	} catch (std::runtime_error& e) {
		std::cerr << "Error loading " << filename << " (" << e.what() << ")" << std::endl;
	}
}

void Audio::playPreview(std::string const& filename) {
	fadeout();
	try {
		m_mpeg.reset(new CFfmpeg(false, true, filename, m_rs.rate()));
		m_mpeg->seek(30.0);
		m_length = m_mpeg->duration();
		setVolume_internal(m_volumePreview);
	} catch (std::runtime_error& e) {
		std::cerr << "Error loading " << filename << " (" << e.what() << ")" << std::endl;
	}
}

void Audio::stopMusic() {
	m_notes = NULL;
	setVolume_internal(0);
	m_mpeg.reset();
	m_length = getNaN();
	m_prebuffering = true; // For the next song
	m_paused = false;
}

void Audio::fadeout() {
	if (m_crossfade == m_crossbuf.size() && m_mpeg) {
		// Read audio into crossfade buffer
		std::vector<int16_t> buf;
		m_mpeg->audioQueue.tryPop(buf, m_crossbuf.size());
		std::transform(buf.begin(), buf.end(), m_crossbuf.begin(), da::conv_from_s16);
		if (m_volume != 1.0) std::transform(m_crossbuf.begin(), m_crossbuf.end(), m_crossbuf.begin(), std::bind1st(std::multiplies<double>(), m_volume));
		m_crossfade = 0;
	}
	stopMusic();
}

double Audio::getPosition() const {
	if (!m_mpeg) return getNaN();
	return m_mpeg->position();
}

bool Audio::isPlaying() const { return m_mpeg && !m_mpeg->audioQueue.eof(); }

void Audio::seek(double seek_dist) {
	if (!isPlaying()) return;
	int position = clamp(getPosition() + seek_dist, 0.0, m_length - 1.0);
	m_paused = true;
	m_prebuffering = true;
	m_mpeg->seek(position);
	m_paused = false;
}

