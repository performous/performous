#include "audio.hh"

#include "xtime.hh"
#include <functional>
#include <iostream>
#include <cmath>
#include "screen.hh"
#include "songs.hh"

#define LENGTH_ERROR -1

CAudio::CAudio(std::string const& pdev, unsigned int rate):
	m_type(),
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
{
	m_mpeg.reset();
	length = 0;
	m_thread.reset(new boost::thread(boost::ref(*this)));
}

CAudio::~CAudio() {
	{
		boost::mutex::scoped_lock l(m_mutex);
		m_type = QUIT;
		m_cond.notify_one();
	}
	m_thread->join();
	m_mpeg.reset();
}

void CAudio::operator()(da::pcm_data& areas, da::settings const&) {
	boost::mutex::scoped_lock l(m_mutex);
	std::size_t samples = areas.channels * areas.frames;
	unsigned int size = 0;
	static double phase = 0.0;
	double volume = m_volume * (m_synth ? 0.3 : 1.0);
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
	if (m_synth && m_mpeg && !m_paused) {
		Songs const& s = *CScreenManager::getSingletonPtr()->getSongs(); // TODO: Kill ScreenManager
		if (s.empty()) return;
		Song::notes_t const& n = s.current().notes; // TODO: Kill ScreenManager
		double t = m_mpeg->position();
		Song::notes_t::const_iterator it = n.begin();
		while (it != n.end() && it->end < t) ++it;
		if (it == n.end() || it->begin > t) { phase = 0.0; return; }
		double freq = MusicalScale().getNoteFreq(it->note - (s.current().noteMin / 12 * 12) + 12);
		double value = 0.0;
		for (size_t i = 0; i < samples; ++i) {
			if (i % areas.channels == 0) {
				value = 0.3 * std::sin(phase) + 0.3 * std::sin(2 * phase) + 0.1 * std::sin(3 * phase);
				phase += 2.0 * M_PI * freq / 48000.0;
			}
			areas.m_buf[i] += value;
		}
	}
}

void CAudio::operator()() {
	for (;;) {
		try {
			Type type;
			std::string filename;
			{
				boost::mutex::scoped_lock l(m_mutex);
				m_ready = true;
				m_condready.notify_all();
				while (m_type == NONE) m_cond.wait(l);
				m_ready = false;
				type = m_type;
				m_type = NONE;
				filename = m_filename;
			}
			switch (type) {
			  case NONE: // Should not get here...
			  case QUIT: return;
			  case STOP: stopMusic_internal(); break;
			  case PREVIEW:
				// Wait a little while before actually starting
				boost::thread::sleep(now() + 0.35);
				{
					boost::mutex::scoped_lock l(m_mutex);
					// Did we receive another event already?
					if (m_type != NONE) continue;
				}
				playPreview_internal(filename);
				break;
			  case PLAY: playMusic_internal(filename); break;
			}
		} catch (std::exception& e) {
			std::cerr << "Audio error: " << e.what() << std::endl;
		}
	}
}

void CAudio::setVolume_internal(unsigned int volume) {
	if (volume == 0) { m_volume = 0.0; return; }
	m_volume = std::pow(10.0, (volume - 100.0) / 100.0 * 2.0);
}

void CAudio::playMusic_internal(std::string const& filename) {
	m_synth = false;
	setVolume_internal(0);
	length = LENGTH_ERROR;
	m_mpeg.reset();
	m_prebuffering = true;
	m_mpeg.reset(new CFfmpeg(false, true, filename, m_rs.rate()));
	if (m_mpeg->duration() < 0) return;
	length = 1e3 * m_mpeg->duration();
	m_paused = false;
	setVolume_internal(m_volumeMusic);
}

void CAudio::playPreview_internal(std::string const& filename) {
	m_synth = false;
	setVolume_internal(0);
	length = LENGTH_ERROR;
	m_mpeg.reset();
	m_prebuffering = true;
	m_mpeg.reset(new CFfmpeg(false, true, filename, m_rs.rate()));
	m_mpeg->seek(30.0);
	if (m_mpeg->duration() < 0) return;
	length = 1e3 * m_mpeg->duration();
	m_paused = false;
	setVolume_internal(m_volumePreview);
}

double CAudio::getLength_internal() {
	if (length != LENGTH_ERROR) return 1e-3 * length;
	return 0.0;
}

bool CAudio::isPlaying_internal() {
	return m_mpeg && !m_mpeg->audioQueue.eof();
}

void CAudio::stopMusic_internal() {
	m_synth = false;
	m_mpeg.reset();
}

double CAudio::getPosition_internal() {
	double position = 0.0;
	if (m_mpeg) position = m_mpeg->position();
	return position;
}

bool CAudio::isPaused_internal() {
	return m_paused;
}

void CAudio::togglePause_internal() {
	if (!isPlaying_internal()) return;
	m_paused = !m_paused;
}

void CAudio::seek_internal(double seek_dist) {
	if (!isPlaying_internal()) return;
	int position = std::max(0.0, std::min(getLength_internal() - 1.0, getPosition_internal() + seek_dist));
	m_paused = true;
	m_prebuffering = true;
	m_mpeg->seek(position);
	m_paused = false;
}

