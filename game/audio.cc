#include "audio.hh"

#include "configuration.hh"
#include "util.hh"
#include <libda/fft.hpp>  // For M_PI
#include <cmath>
#include <iostream>
#include <boost/filesystem.hpp>

struct SampleStream {
	SampleStream(boost::shared_ptr<FFmpeg> const& mpeg): m_mpeg(mpeg) {}
	bool operator()(da::pcm_data& data) {
		m_mpeg->audioQueue(data, m_pos);
		return !m_mpeg->audioQueue.eof(m_pos);;
	}
  private:
	boost::shared_ptr<FFmpeg> m_mpeg;
	int64_t m_pos;
};

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
	std::size_t samples = areas.samples();
	static double phase = 0.0;
	// Synthesize tones
	Notes const* n = m_notes;
	double t = getPosition();
	if (n) {
		Notes::const_iterator it = n->begin();
		while (it != n->end() && it->end < t) ++it;
		for (size_t i = 0; i < samples; ++i) areas.rawbuf[i] *= 0.3; // Decrease music volume
		if (it == n->end() || it->type == Note::SLEEP || it->begin > t) { phase = 0.0; return true; }
		int note = it->note % 12;
		double d = (note + 1) / 13.0;
		double freq = MusicalScale().getNoteFreq(note + 12);
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
	// Set chain volume
	double vol = config[m_volumeSetting].i();
	if (vol > 0) vol = std::pow(10.0, (vol - 100.0) / 100.0 * 2.0);
	m_volume.level(vol);
	return true;
}

void Audio::play(Sample const& s, std::string const& volumeSetting) {
	double vol = config[volumeSetting].i();
	if (vol > 0) vol = std::pow(10.0, (vol - 100.0) / 100.0 * 2.0);
	m_volume.level(vol);
	boost::shared_ptr<da::accumulate> acc(new da::accumulate());
	boost::shared_ptr<da::chain> ch(new da::chain());
	ch->add(da::shared_ref(new SampleStream(s.mpeg)));
	ch->add(da::shared_ref(new da::volume(vol)));
	acc->add(da::shared_ref(ch));
	m_mixer.add(da::shared_ref(acc));
}

void Audio::playMusic(std::map<std::string,std::string> const& filenames, bool preview, double fadeTime, double startPos, int speed) {
	if (!isOpen()) return;
	da::lock_holder l = m_mixer.lock();
	fadeout(fadeTime);
	boost::shared_ptr<da::chain> ch(new da::chain());
	for(std::map<std::string,std::string>::const_iterator it = filenames.begin() ; it != filenames.end() ; ++it ) {
		std::string f;
		try {
			f = it->second;
			if (speed != 100) {
				// instead of real-time time-stretching we use previously created files
				// best effects have been achieved with SBSMS library
				// as used by Audacity's Sliding Time Scale/ Pitch Shift plugin
				std::string f2 = "-" + boost::lexical_cast<std::string>(speed) + ".ogg";
				f.replace(f.find(".ogg"), 4, f2);
				// std::cout << "loading time-stretched audio (" << f << ")" << std::endl;

				if (!boost::filesystem::exists(f)) throw std::runtime_error("time-stretched file \"" + f +"\" not found");
			}

			boost::shared_ptr<Stream> s(new Stream(f, m_rs.rate()));
			m_streams[it->first] = s;
			ch->add(da::shared_ref(s));
			s->seek(startPos);
		} catch (std::runtime_error& e) {
			if (speed != 100) {
				// in case time-stretching failed let's retry at normal speed
				// TODO: we could also auto-create time-stretched files here
				playMusic(filenames, preview, fadeTime, startPos, 100);
				return;
			}
			// in case we are at normal speed we skip this file and try to continue
			std::cerr << "Error loading " << f << " (" << e.what() << ")" << std::endl;
			continue;
		}
	}
	ch->add(boost::ref(*this));
	m_volumeSetting = preview ? "audio/preview_volume" : "audio/music_volume";
	ch->add(boost::ref(m_volume));
	m_mixer.fadein(da::shared_ref(ch), fadeTime, startPos);
	if (!preview) pause(false);

	m_speed = speed;
}

void Audio::playMusic(std::string const& filename, bool preview, double fadeTime, double startPos, int speed) {
	std::map<std::string,std::string> tmp;
	tmp["unidentified"] = filename;
	playMusic(tmp, preview, fadeTime, startPos, speed);
}

void Audio::stopMusic() {
	da::lock_holder l = m_mixer.lock();
	m_notes = NULL;
	m_mixer.clear();
	m_streams.clear();
}

void Audio::fadeout(double fadeTime) {
	da::lock_holder l = m_mixer.lock();
	m_notes = NULL;
	m_mixer.fadeout(fadeTime);
	m_streams.clear();
}

double Audio::getPosition() const {
	da::lock_holder l = m_mixer.lock();
	// only the audio slows down; the game still uses original time base
	return m_streams.empty() ? getNaN() : m_streams.begin()->second->pos() * m_speed/100.0;
}

double Audio::getLength() const {
	da::lock_holder l = m_mixer.lock();
	return m_streams.empty() ? getNaN() : m_streams.begin()->second->duration() * m_speed/100.0;
}

bool Audio::isPlaying() const {
	da::lock_holder l = m_mixer.lock();
	return m_streams.empty() ? false : !m_streams.begin()->second->eof();
}

void Audio::seek(double offset2) {
	double offset = offset2 * 100.0/m_speed;
	da::lock_holder l = m_mixer.lock();
	for(std::map<std::string,boost::shared_ptr<Stream> >::iterator it = m_streams.begin() ; it != m_streams.end() ; ++it)
		it->second->seek(clamp(it->second->pos() + offset, 0.0, it->second->duration()));
	pause(false);
}

void Audio::seekPos(double pos2) {
	double pos = pos2 * 100.0/m_speed;
	da::lock_holder l = m_mixer.lock();
	for(std::map<std::string,boost::shared_ptr<Stream> >::iterator it = m_streams.begin() ; it != m_streams.end() ; ++it)
		it->second->seek(pos);
	pause(false);
}

void Audio::pause(bool state) {
	da::lock_holder l = m_mixer.lock();
	m_paused = state;
	m_mixer.pause(m_paused);
}

void Audio::streamFade(std::string stream_id, double level) {
	da::lock_holder l = m_mixer.lock();
	std::map<std::string,boost::shared_ptr<Stream> >::iterator it = m_streams.find(stream_id);
	if( it != m_streams.end() ) it->second->fade = level;
}

