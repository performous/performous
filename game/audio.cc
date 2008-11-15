#include "audio.hh"

#include "xtime.hh"
#include <functional>
#include <iostream>
#include <cmath>
#include "screen.hh"

Audio::Audio(std::string const& pdev, unsigned int rate):
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
	// TODO: fadeout();
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
	m_length = std::numeric_limits<double>::quiet_NaN();
	m_prebuffering = true; // For the next song
	m_paused = false;
}

double Audio::getPosition() const {
	if (!m_mpeg) return std::numeric_limits<double>::quiet_NaN();
	return m_mpeg->position();
}

bool Audio::isPlaying() const { return m_mpeg && !m_mpeg->audioQueue.eof(); }

void Audio::seek(double seek_dist) {
	if (!isPlaying()) return;
	int position = std::max(0.0, std::min(m_length - 1.0, getPosition() + seek_dist));
	m_paused = true;
	m_prebuffering = true;
	m_mpeg->seek(position);
	m_paused = false;
}

