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

Audio::Audio():
	m_paused(false),
	m_need_resync(false)
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
		if(m_need_resync) {
			std::cout << "Audio need to be synched here" << std::endl;
			/*
			// do a fast forward sync
			double position = 0.0;
			for (Streams::iterator it = m_streams.begin(); it != m_streams.end(); ++it) {
				if( it->fadingout() ) continue;
				double tmp = it->mpeg.position();
				if( tmp > position ) position = tmp;
			}
			for (Streams::iterator it = m_streams.begin(); it != m_streams.end(); ++it) {
				it->mpeg.seek(position);
			}
			*/
			m_need_resync = false;
		}
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

void Audio::playMusic(std::vector<std::string> const& filenames, bool preview, double fadeTime, double startPos) {
	if (!isOpen()) return;
	// First construct the new stream
	boost::recursive_mutex::scoped_lock l(m_mutex);
	fadeout(fadeTime);
	for(std::vector<std::string>::const_iterator it = filenames.begin() ; it != filenames.end() ; ++it ) {
		std::auto_ptr<Stream> s;
		try {
			s.reset(new Stream(*it, m_rs.rate(), preview ? config["audio/preview_volume"] : config["audio/music_volume"]));
			s->fadein(fadeTime);
			if (startPos != 0.0) s->mpeg.seek(startPos, false);
		} catch (std::runtime_error& e) {
			std::cerr << "Error loading " << *it << " (" << e.what() << ")" << std::endl;
			continue;
		}
		m_streams.push_back(s);
	}
	if (!preview) m_paused = false;
	m_need_resync = true;
}

void Audio::playMusic(std::string const& filename, bool preview, double fadeTime, double startPos) {
	if (!isOpen()) return;
	// First construct the new stream
	std::auto_ptr<Stream> s;
	try {
		s.reset(new Stream(filename, m_rs.rate(), preview ? config["audio/preview_volume"] : config["audio/music_volume"]));
		s->fadein(fadeTime);
		if (startPos != 0.0) s->mpeg.seek(startPos, false);
	} catch (std::runtime_error& e) {
		std::cerr << "Error loading " << filename << " (" << e.what() << ")" << std::endl;
		return;
	}
	boost::recursive_mutex::scoped_lock l(m_mutex);
	fadeout(fadeTime);
	m_streams.push_back(s);
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

void Audio::seek(double offset) {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	if (m_streams.empty()) return;
	Stream& s = m_streams.back();
	seekPos(s.mpeg.position() + offset);
}

void Audio::seekPos(double pos) {
	boost::recursive_mutex::scoped_lock l(m_mutex);
	if (m_streams.empty()) return;
	for (Streams::iterator it = m_streams.begin(); it != m_streams.end(); ++it) {
		int position = clamp(pos, 0.0, it->mpeg.duration() - 1.0);
		it->mpeg.seek(position);
		it->prebuffering = true;
	}
	m_paused = false;
	m_need_resync = true;
}

