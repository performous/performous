#include "audio.hh"

#include "configuration.hh"
#include "screen.hh"
#include "util.hh"
#include "xtime.hh"
#include <libda/fft.hpp>  // For M_PI
#include <cmath>
#include <functional>
#include <iostream>
#include <memory>

Audio::Audio(): m_paused(false) {
	m_mixer.add(boost::ref(*this));
}

void Audio::open(std::string const& pdev, std::size_t rate, std::size_t frames) {
	m_mixer.stop();
	stopMusic();
	m_rs = da::settings(pdev)
	  .set_channels(2)
	  .set_rate(rate)
	  .set_frames(frames)
	  .set_debug(std::cerr);
	m_mixer.start(m_rs);
}

bool Audio::operator()(da::pcm_data& areas) {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	std::size_t samples = areas.channels * areas.frames;
	static double phase = 0.0;
	// Synthesize tones
	Notes const* n = m_notes;
	if (n && !m_paused) {
		double t = getPosition();
		Notes::const_iterator it = n->begin();
		while (it != n->end() && it->end < t) ++it;
		for (size_t i = 0; i < samples; ++i) areas.rawbuf[i] *= 0.3; // Decrease music volume
		if (it == n->end() || it->type == Note::SLEEP || it->begin > t) { phase = 0.0; return true; }
		int n = it->note % 12;
		double d = (n + 1) / 13.0;
		double freq = MusicalScale().getNoteFreq(n + 12);
		double value = 0.0;
		// Synthesize tones
		for (size_t i = 0; i < samples; ++i) {
			if (i % areas.channels == 0) {
				value = d * 0.2 * std::sin(phase) + 0.2 * std::sin(2 * phase) + (1.0 - d) * 0.2 * std::sin(4 * phase);
				phase += 2.0 * M_PI * freq / m_rs.rate();
			}
			areas.rawbuf[i] += value;
		}
	}
	return true;
}

void Audio::playMusic(std::vector<std::string> const& filenames, bool preview, double fadeTime, double startPos) {
	if (!isOpen()) return;
	boost::recursive_mutex::scoped_lock l(m_mutex);
	// First construct the new stream
	fadeout(fadeTime);
	for(std::vector<std::string>::const_iterator it = filenames.begin() ; it != filenames.end() ; ++it ) {
		try {
			m_streams.push_back(boost::shared_ptr<Stream>(new Stream(*it, m_rs.rate(), preview ? config["audio/preview_volume"] : config["audio/music_volume"])));
			m_streams.back()->fadein(fadeTime);
			if (startPos != 0.0) m_streams.back()->mpeg.seek(startPos, false);
		} catch (std::runtime_error& e) {
			std::cerr << "Error loading " << *it << " (" << e.what() << ")" << std::endl;
			continue;
		}
	}
	for (size_t i = 0; i < m_streams.size(); ++i) m_mixer.add(da::shared_ref(m_streams[i]));
	if (!preview) m_paused = false;
}

void Audio::playMusic(std::string const& filename, bool preview, double fadeTime, double startPos) {
	playMusic(std::vector<std::string>(1, filename), preview, fadeTime, startPos);
}

void Audio::stopMusic() {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	m_notes = NULL;
	m_mixer.fade(0.0);
	m_streams.clear();
}

void Audio::fadeout(double fadeTime) {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	m_notes = NULL;
	m_mixer.fade(fadeTime);
	m_streams.clear();
}

double Audio::getPosition() const {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	return m_streams.empty() ? getNaN() : m_streams.front()->mpeg.audioQueue.position();
}

double Audio::getLength() const {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	return m_streams.empty() ? getNaN() : m_streams.front()->mpeg.duration();
}

bool Audio::isPlaying() const {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	return m_streams.empty() ? getNaN() : !m_streams.front()->mpeg.audioQueue.eof();
}

void Audio::playSample(std::string filename) {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	/* FIXME if( m_samples.find(filename) == m_samples.end() ) {
		m_samples.insert(filename,new AudioSample(filename, m_rs.rate())); 
	} else {
		m_samples[filename].reset_position();
	}*/
}

void Audio::seek(double offset) {
	seekPos(m_streams.back()->mpeg.position() + offset);
}

void Audio::seekPos(double pos) {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	for (size_t i = 0; i < m_streams.size(); ++i) {
		Stream& s = *m_streams[i];
		int position = clamp(pos, 0.0, s.mpeg.duration() - 1.0);
		s.mpeg.seek(position);
		s.prebuffering = true;
	}
	m_paused = false;
}

