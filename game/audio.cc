#include "audio.hh"

#include <cmath>
#include <functional>
#include <iostream>
#include <memory>
#include "screen.hh"
#include "configuration.hh"
#include "util.hh"
#include "xtime.hh"

Audio::Audio():
	m_paused(false)
{}

void Audio::open(std::string const& pdev, std::size_t rate, std::size_t frames) {
	m_playback.reset();
	stopMusic();
	m_rs = da::settings(pdev)
	  .set_callback(boost::ref(*this))
	  .set_channels(2)
	  .set_rate(rate)
	  .set_frames(frames)
	  .set_debug(std::cerr);
	m_playback.reset(new da::playback(m_rs));
}

void Audio::operator()(da::pcm_data& areas, da::settings const&) {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	std::size_t samples = areas.channels * areas.frames;
	static double phase = 0.0;
	std::fill(areas.m_buf, areas.m_buf + samples, 0.0f);
	if (!m_paused) {
		for (Streams::iterator it = m_streams.begin(); it != m_streams.end();) {
			it->playmix(areas.m_buf, samples);
			if (it->fade <= 0.0) { it = m_streams.erase(it); continue; }
			++it;
		}
	}
	for (boost::ptr_map<std::string, AudioSample>::iterator it = m_samples.begin(); it != m_samples.end();++it) {
		it->second->playmix(areas.m_buf, samples);
	}
	// Synthesize tones
	Notes const* n = m_notes;
	if (n && !m_paused) {
		double t = getPosition();
		Notes::const_iterator it = n->begin();
		while (it != n->end() && it->end < t) ++it;
		for (size_t i = 0; i < samples; ++i) areas.m_buf[i] *= 0.3; // Decrease music volume
		if (it == n->end() || it->type == Note::SLEEP || it->begin > t) { phase = 0.0; return; }
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
			areas.m_buf[i] += value;
		}
	}
}

void Audio::setVolume_internal(unsigned int volume) {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	if (m_streams.empty()) return;
	Stream& s = m_streams.back();
	if (volume == 0) { s.volume = 0.0; return; }
	s.volume = std::pow(10.0, (volume - 100.0) / 100.0 * 2.0);
}

void Audio::playMusic(std::string const& filename, bool preview, double fadeTime, double startPos) {
	if (!isOpen()) return;
	// First construct the new stream
	std::auto_ptr<Stream> s;
	try {
		s.reset(new Stream(filename, m_rs.rate()));
		s->fadein(fadeTime);
		if (startPos != 0.0) s->mpeg.seek(startPos, false);
	} catch (std::runtime_error& e) {
		std::cerr << "Error loading " << filename << " (" << e.what() << ")" << std::endl;
		return;
	}
	boost::recursive_mutex::scoped_lock l(m_mutex);
	fadeout(fadeTime);
	m_streams.push_back(s);
	setVolume_internal(preview ? config["audio/preview_volume"].get_i() : config["audio/music_volume"].get_i());
	if (!preview) m_paused = false;
}

void Audio::stopMusic() {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	m_notes = NULL;
	m_streams.clear();
}

void Audio::fadeout(double fadeTime) {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	m_notes = NULL;
	for (Streams::iterator it = m_streams.begin(); it != m_streams.end(); ++it) it->fadeout(fadeTime);
}

double Audio::getPosition() const {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	return m_streams.empty() ? getNaN() : m_streams.back().mpeg.audioQueue.position();
}

double Audio::getLength() const {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	return m_streams.empty() ? getNaN() : m_streams.back().mpeg.duration();
}

bool Audio::isPlaying() const {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	return m_streams.empty() ? getNaN() : !m_streams.back().mpeg.audioQueue.eof();
}

void Audio::playSample(std::string filename) {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	if( m_samples.find(filename) == m_samples.end() ) {
		m_samples.insert(filename,new AudioSample(filename, m_rs.rate())); 
	} else {
		m_samples[filename].reset_position();
	}
}

void Audio::seek(double seek_dist) {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	if (m_streams.empty()) return;
	Stream& s = m_streams.back();
	int position = clamp(s.mpeg.position() + seek_dist, 0.0, s.mpeg.duration() - 1.0);
	s.mpeg.seek(position);
	s.prebuffering = true;
	m_paused = false;
}

