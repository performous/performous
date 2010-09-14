#include "audio.hh"

#include "configuration.hh"
#include "util.hh"
#include "libda/fft.hpp"  // For M_PI
#include "libda/portaudio.hpp"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <iostream>
#include <map>

using namespace boost::posix_time;

namespace {
	/**
	 * A function to parse key=value pairs with quoting capabilites.
	 */
	std::map<std::string, std::string> parseKeyValuePairs(const std::string& st) {
		std::map<std::string, std::string> ret;
		bool inside_quotes = false;
		int parsing_key = true;
		std::string key = "", value = "";
		for (size_t i = 0; i < st.size(); ++i) {
			// Quotes
			if (st[i] == '"') { inside_quotes = !inside_quotes; continue; }
			// Value end
			if (st[i] == ' ' && !inside_quotes && !parsing_key) {
				if (value.empty()) continue; // Skip whitespace after equals sign
				ret[key] = value;
				key = ""; value = "";
				parsing_key = true;
				continue;
			}
			// Space in key (bad)
			if (st[i] == ' ' && parsing_key && !key.empty())
				throw std::logic_error("Space in key in string: " + st);
			// Value start
			if (st[i] == '=' && !inside_quotes) {
				if (key.empty()) throw std::logic_error("Empty key in string: " + st);
				parsing_key = false;
				continue;
			}
			// Key start
			if (st[i] != ' ' && parsing_key) { key += st[i]; continue; }
			// If we got here, it is value
			value += st[i];
		}
		// Handle last key
		if (!key.empty()) ret[key] = value;
		return ret;
	}

	/**
	 * Pitch shifter.
	 * @param begin start of the buffer
	 * @param end end of the buffer
	 * @param factor shift factor from 0 (no shift) to 1 (maximum)
	 */
	template <typename It> void PitchShift(It begin, It end, double factor) {
		//FIXME: Dummy volume bender
		for (It i = begin; i != end; ++i)
			*i *= factor * factor; // Decrease music volume
	}
}
/**
* Advanced audio sync code.
* Produces precise monotonic clock synced to audio output callback (which may suffer of major jitter).
* Uses system clock as timebase but the clock is skewed (made slower or faster) depending on whether
* it is late or early. The clock is also stopped if audio output pauses.
**/
class AudioClock {
	static ptime getTime() { return microsec_clock::universal_time(); }
	// Conversion helpers
	static double getSeconds(time_duration t) { return 1e-6 * t.total_microseconds(); }
	static time_duration getDuration(double seconds) { return microseconds(1e6 * seconds); }

	mutable boost::mutex m_mutex;
	ptime m_baseTime; ///< A reference time (corresponds to m_basePos)
	double m_basePos; ///< A reference position in song
	double m_skew; ///< The skew ratio applied to system time (since baseTime)
	volatile float m_max; ///< Maximum output value for the clock (end of the current audio block)
	/// Get the current position (current time via parameter, no locking)
	double pos_internal(ptime now) const {
		double t = m_basePos + (1.0 + m_skew) * getSeconds(now - m_baseTime);
		return std::min<double>(t, m_max);
	}
public:
	AudioClock(): m_baseTime(), m_basePos(), m_skew(), m_max() {}
	/**
	* Called from audio callback to keep the clock synced.
	* @param audioPos the current position in the song
	* @param length the duration of the current audio block
	*/
	void timeSync(time_duration audioPos, time_duration length) {
		const double maxError = 0.1;  // Step the clock instead of skewing if over 100 ms off
		const double fudgeFactor = 0.1;  // Adjustment ratio
		// Full correction requires locking, but we can update max without lock too
		boost::mutex::scoped_try_lock l(m_mutex, boost::defer_lock);
		double max = getSeconds(audioPos + length);
		if (!l.try_lock()) {
			if (max > m_max) m_max = max; // Allow increasing m_max
			return;
		}
		// Mutex locked - do a full update
		ptime now = getTime();
		double sys = pos_internal(now);  // Current position (based on system clock + corrections)
		double audio = getSeconds(audioPos);  // Audio time
		double diff = audio - sys;
		// Skew-based correction only if going forward and relatively well synced
		if (max > m_max && std::abs(diff) < maxError) {
			// Update base position (this should not affect the clock)
			m_baseTime = now;
			m_basePos = sys;
			// Apply a VERY ARTIFICIAL correction for clock!
			double valadj = getSeconds(length) * 0.1 * rand() / RAND_MAX;  // Dither
			m_skew = 0.99 * m_skew + 0.01 * (diff < valadj ? -1.0 : 1.0) * fudgeFactor;
			// Limits to keep things sane in abnormal situations
			if (m_skew > 0.01) m_skew = 0.01;
			else if (m_skew < -0.01) m_skew = -0.01;
		} else {
			// Off too much, step to correct time
			m_baseTime = now;
			m_basePos = audio;
			m_skew = 0.0;
		}
		m_max = max;
	}
	/// Get the current position in seconds
	double pos() const {
		boost::mutex::scoped_lock l(m_mutex);
		return pos_internal(getTime());
	}
};

class Music {
	struct Track {
		FFmpeg mpeg;
		float fadeLevel;
		float pitchFactor;
		Track(std::string const& filename, unsigned int sr): mpeg(false, true, filename, sr), fadeLevel(1.0f), pitchFactor(0.0f) {}
	};
	typedef boost::ptr_map<std::string, Track> Tracks;
	Tracks tracks; ///< Audio decoders
	double srate; ///< Sample rate
	int64_t m_pos; ///< Current sample position
	bool m_preview;
	AudioClock m_clock;
	time_duration durationOf(int64_t samples) const { return microseconds(1e6 * samples / srate / 2.0); }
public:
	double fadeLevel;
	double fadeRate;
	typedef std::map<std::string,std::string> Files;
	typedef std::vector<float> Buffer;
	Music(Files const& filenames, unsigned int sr, bool preview):
	  srate(sr), m_pos(), m_preview(preview), fadeLevel(), fadeRate()
	{
		for (Files::const_iterator it = filenames.begin(), end = filenames.end(); it != end; ++it) {
			if (it->second.empty()) continue; // Skip tracks with no filenames; FIXME: Why do we even have those here, shouldn't they be eliminated earlier?
			tracks.insert(it->first, std::auto_ptr<Track>(new Track(it->second, sr)));
		}
	}
	/// Sums the stream to output sample range, returns true if the stream still has audio left afterwards.
	bool operator()(float* begin, float* end) {
		size_t samples = end - begin;
		m_clock.timeSync(durationOf(m_pos), durationOf(samples)); // Keep the clock synced
		bool eof = true;
		Buffer mixbuf(end - begin);
		for (Tracks::iterator it = tracks.begin(), itend = tracks.end(); it != itend; ++it) {
			Track& t = *it->second;
// FIXME: Include this code bit once there is a sane pitch shifting algorithm
#if 0
//			if (it->first == "guitar") std::cout << t.pitchFactor << std::endl;
			if (t.pitchFactor != 0) { // Pitch shift
				Buffer tempbuf(end - begin);
				// Get audio to temp buffer
				if (t.mpeg.audioQueue(&*tempbuf.begin(), &*tempbuf.end(), m_pos, t.fadeLevel)) eof = false;
				// Do the magic
				PitchShift(&*tempbuf.begin(), &*tempbuf.end(), t.pitchFactor);
				// Mix with other tracks
				Buffer::iterator m = mixbuf.begin();
				Buffer::iterator b = tempbuf.begin();
				while (b != tempbuf.end())
					*m++ += (*b++);
			// Otherwise just get the audio and mix it straight away
			} else
#endif
			if (t.mpeg.audioQueue(&*mixbuf.begin(), &*mixbuf.end(), m_pos, t.fadeLevel)) eof = false;
		}
		m_pos += samples;
		for (size_t i = 0, iend = mixbuf.size(); i != iend; ++i) {
			if (i % 2 == 0) {
				fadeLevel += fadeRate;
				if (fadeLevel <= 0.0) return false;
				if (fadeLevel > 1.0) { fadeLevel = 1.0; fadeRate = 0.0; }
			}
			begin[i] += mixbuf[i] * fadeLevel * static_cast<float>(m_preview ? config["audio/preview_volume"].i() : config["audio/music_volume"].i())/100.0;
		}
		return !eof;
	}
	void seek(double time) { m_pos = time * srate * 2.0; }
	/// Get the current position in seconds
	double pos() const { return m_clock.pos(); }
	double duration() const {
		double dur = 0.0;
		for (Tracks::const_iterator it = tracks.begin(), itend = tracks.end(); it != itend; ++it) {
			dur = std::max(dur, it->second->mpeg.audioQueue.duration());
		}
		return dur;
	}
	/// Prepare (seek) all tracks to current position, return true when done (nonblocking)
	bool prepare() {
		bool ready = true;
		for (Tracks::iterator it = tracks.begin(), itend = tracks.end(); it != itend; ++it) {
			if (!it->second->mpeg.audioQueue.prepare(m_pos)) ready = false;
		}
		return ready;
	}
	void trackFade(std::string const& name, double fadeLevel) {
		Tracks::iterator it = tracks.find(name);
		if (it == tracks.end()) return;
		it->second->fadeLevel = fadeLevel;
	}
	void trackPitchBend(std::string const& name, double pitchFactor) {
		Tracks::iterator it = tracks.find(name);
		if (it == tracks.end()) return;
		it->second->pitchFactor = pitchFactor;
	}
};

struct Sample {
  private:
	unsigned int srate;
	double m_pos;
	FFmpeg mpeg;
	bool eof;
  public:
	Sample(std::string const& filename, unsigned sr) : srate(sr), m_pos(), mpeg(false, true, filename, sr), eof(true) { }
	void operator()(float* begin, float* end) {
		if(eof) {
			// No more data to play in this sample
			return;
		}
		std::vector<float> mixbuf(end - begin);
		if(!mpeg.audioQueue(&*mixbuf.begin(), &*mixbuf.end(), m_pos, 1.0)) {
			eof = true;
		}
		for (size_t i = 0, iend = end - begin; i != iend; ++i) {
			begin[i] += mixbuf[i] * static_cast<float>(config["audio/fail_volume"].i())/100.0;
		}
		m_pos += end - begin;
	}
	void reset() {
		eof = false;
		m_pos = 0;
	}
};

struct Synth {
  private:
	Notes m_notes;
	double srate; ///< Sample rate
  public:
	Synth(Notes const& notes, unsigned int sr) : m_notes(notes), srate(sr) {};
	void operator()(float* begin, float* end, double position) {
		static double phase = 0.0;
		for (float *i = begin; i < end; ++i) *i *= 0.3; // Decrease music volume

		std::vector<float> mixbuf(end - begin);
		Notes::const_iterator it = m_notes.begin();

		while (it != m_notes.end() && it->end < position) ++it;
		if (it == m_notes.end() || it->type == Note::SLEEP || it->begin > position) { phase = 0.0; return; }
		int note = it->note % 12;
		double d = (note + 1) / 13.0;
		double freq = MusicalScale().getNoteFreq(note + 12);
		double value = 0.0;
		// Synthesize tones
		for (size_t i = 0, iend = mixbuf.size(); i != iend; ++i) {
			if (i % 2 == 0) {
				value = d * 0.2 * std::sin(phase) + 0.2 * std::sin(2 * phase) + (1.0 - d) * 0.2 * std::sin(4 * phase);
				phase += 2.0 * M_PI * freq / srate;
			}
			begin[i] += value;
		}
	}
};

struct Command {
	enum { TRACK_FADE, TRACK_PITCHBEND, SAMPLE_RESET } type;
	std::string track;
	double factor;
};

struct Output {
	boost::mutex mutex;
	boost::mutex samples_mutex;
	boost::mutex synth_mutex;
	std::auto_ptr<Synth> synth;
	std::auto_ptr<Music> preloading;
	boost::ptr_vector<Music> playing, disposing;
	boost::ptr_map<std::string, Sample> samples;
	std::vector<Command> commands;
	volatile bool paused;
	Output(): paused(false) {}

	void callbackUpdate() {
		boost::mutex::scoped_try_lock l(mutex);
		if (!l.owns_lock()) return;  // No update now, try again later (cannot stop and wait for mutex to be released)
		// Move from preloading to playing, if ready
		if (preloading.get() && preloading->prepare()) {
			if (!playing.empty()) playing[0].fadeRate = -preloading->fadeRate;  // Fade out the old music
			playing.insert(playing.begin(), preloading);
		}
		// Process commands
		for (size_t i = 0; i < commands.size(); ++i) {
			Command const& cmd = commands[i];
			switch (cmd.type) {
			case Command::TRACK_FADE:
				if (!playing.empty()) playing[0].trackFade(cmd.track, cmd.factor);
				break;
			case Command::TRACK_PITCHBEND:
				if (!playing.empty()) playing[0].trackPitchBend(cmd.track, cmd.factor);
				break;
			case Command::SAMPLE_RESET:
				boost::ptr_map<std::string, Sample>::iterator it = samples.find(cmd.track);
				if (it != samples.end())
					it->second->reset();
				break;
			}
		}
		commands.clear();
	}

	void callback(float* begin, float* end) {
		callbackUpdate();
		std::fill(begin, end, 0.0f);
		if (paused) return;
		// Mix in from the streams currently playing
		for (size_t i = 0; i < playing.size();) {
			bool keep = playing[i](begin, end);  // Do the actual mixing
			boost::mutex::scoped_try_lock l(mutex, boost::defer_lock);
			if (!keep && l.try_lock()) {
				// Dispose streams no longer needed by moving them to another container (that will be cleared by another thread).
				disposing.transfer(disposing.end(), playing.begin() + i, playing);
				continue;
			}
			++i;
		}
		// Mix in the samples currently playing
		{
			// samples should not be created/destroyed on the fly
			boost::mutex::scoped_try_lock l(samples_mutex, boost::defer_lock);
			if(l.try_lock()) {
				for(boost::ptr_map<std::string, Sample>::iterator it = samples.begin() ; it != samples.end() ; ++it) {
					(*it->second)(begin, end);
				}
			}
		}
		// Mix synth if available (should be done at the end)
		{
			boost::mutex::scoped_try_lock l(synth_mutex, boost::defer_lock);
			if(l.try_lock() && synth.get() && !playing.empty()) {
				(*synth.get())(begin, end, playing[0].pos());
			}
		}
	}
};


Device::Device(unsigned int in, unsigned int out, double rate, unsigned int dev):
  in(in), out(out), rate(rate), dev(dev),
  stream(*this, in ? portaudio::Params().channelCount(in).device(dev).suggestedLatency(config["audio/latency"].f()): (const PaStreamParameters*)NULL,
	(out ? portaudio::Params().channelCount(out).device(dev).suggestedLatency(config["audio/latency"].f()) : (const PaStreamParameters*)NULL), rate),
  outptr()
{
	mics.resize(in);
}

void Device::start() {
	PaError err = Pa_StartStream(stream);
	if (err != paNoError) throw std::runtime_error("Cannot start PortAudio audio stream "
	  + boost::lexical_cast<std::string>(dev) + ": " + Pa_GetErrorText(err));
}

int Device::operator()(void const* input, void* output, unsigned long frames, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags) try {
	float const* inbuf = static_cast<float const*>(input);
	float* outbuf = static_cast<float*>(output);
	for (std::size_t i = 0; i < mics.size(); ++i) {
		if (!mics[i]) continue;  // No analyzer? -> Channel not used
		da::sample_const_iterator it = da::sample_const_iterator(inbuf + i, in);
		mics[i]->input(it, it + frames);
	}
	if (outptr) outptr->callback(outbuf, outbuf + 2 * frames);
	return paContinue;
} catch (std::exception& e) {
	std::cerr << "Exception in audio callback: " << e.what() << std::endl;
	return paAbort;
}



struct Audio::Impl {
	Output output;
	portaudio::Init init;
	boost::ptr_vector<Device> devices;
	boost::ptr_vector<Analyzer> analyzers;
	bool playback;
	Impl(): init(), playback() {
		// Parse audio devices from config
		ConfigItem::StringList devs = config["audio/devices"].sl();
		for (ConfigItem::StringList::const_iterator it = devs.begin(), end = devs.end(); it != end; ++it) {
			try {
				struct Params {
					int out;
					unsigned int rate;
					std::string dev;
					std::vector<std::string> mics;
				} params = Params();
				params.out = 0;
				params.rate = 48000;
				// Break into tokens:
				std::map<std::string, std::string> keyvalues = parseKeyValuePairs(*it);
				for (std::map<std::string, std::string>::const_iterator it2 = keyvalues.begin();
				  it2 != keyvalues.end(); ++it2) {
					// Handle keys
					std::string key = it2->first;
					std::istringstream iss(it2->second);
					if (key == "out") iss >> params.out;
					else if (key == "rate") iss >> params.rate;
					else if (key == "dev") std::getline(iss, params.dev);
					else if (key == "mics") {
						// Parse a comma-separated list of mics
						for (std::string mic; std::getline(iss, mic, ','); params.mics.push_back(mic)) {}
					}
					else throw std::runtime_error("Unknown device parameter " + key);
					if (!iss.eof()) throw std::runtime_error("Syntax error parsing device parameter " + key);
				}
				int count = Pa_GetDeviceCount();
				int dev = -1;
				// Handle empty device
				if (params.dev.empty()) dev = (params.out == 0 ? Pa_GetDefaultInputDevice() : Pa_GetDefaultOutputDevice());
				// Try numeric value
				if (dev < 0) {
					std::istringstream iss(params.dev);
					int tmp;
					if (iss >> tmp && iss.get() == EOF && tmp >= 0 && tmp < count) dev = tmp;
				}
				std::clog << "audio/info: Trying audio device \"" << params.dev << "\", id: " << dev
					<< ", in: " << params.mics.size() << ", out: " << params.out << std::endl;
				bool skip_partial = false;
				bool found = false;
				// Try exact match first, then partial
				for (int match_partial = 0; match_partial < 2 && !skip_partial; ++match_partial) {
					// Loop through the devices and try everything that matches the name
					for (int i = -1; i < count && (dev < 0 || i == -1); ++i) {
						if (dev >= 0 && i == -1) i = dev;
						else if (i == -1) continue;
						PaDeviceInfo const* info = Pa_GetDeviceInfo(i);
						if (!info) continue;
						if (info->maxInputChannels < int(params.mics.size())) continue;
						if (info->maxOutputChannels < params.out) continue;
						if (dev < 0) { // Try matching by name
							if (!match_partial && info->name != params.dev) continue;
							if (match_partial && std::string(info->name).find(params.dev) == std::string::npos) continue;
						}
						// Match found if we got here
						int assigned_mics = 0;
						bool device_init_threw = true;
						try {
							devices.push_back(new Device(params.mics.size(), params.out, params.rate, i));
							Device& d = devices.back();
							device_init_threw = false;
							// Start capture/playback on this device
							d.start();
							// Assign mics for all channels of the device
							for (unsigned int j = 0; j < d.in; ++j) {
								if (analyzers.size() >= 4) break; // Too many mics
								std::string const& m = params.mics[j];
								if (m.empty()) continue; // Input channel not used
								// Check that the color is not already taken
								bool mic_used = false;
								for (size_t mi = 0; mi < analyzers.size(); ++mi) {
									if (analyzers[mi].getId() == m) { mic_used = true; break; }
								}
								if (mic_used) continue;
								// Add the new analyzer
								Analyzer* a = new Analyzer(d.rate, m);
								analyzers.push_back(a);
								d.mics[j] = a;
								++assigned_mics;
							}
							// Assign playback output for the first available stereo output
							if (!playback && d.out == 2) { d.outptr = &output; playback = true; }
						} catch (...) {
							if (!device_init_threw) devices.pop_back();
							if (dev > 0) { skip_partial = true; break; } // Numeric, end search
							continue;
						}
						skip_partial = true;
						found = true;
						std::clog << "audio/info: Using audio device: " << i;
						if (assigned_mics) std::clog << ", input channels: " << assigned_mics;
						if (params.out) std::clog << ", output channels: " << params.out;
						std::clog << std::endl;
						break;
					}
				}
				// Error handling
				if (!found) throw std::runtime_error("Not found or already in use.");
			} catch(std::runtime_error& e) {
				std::clog << "audio/error: Audio device '" << *it << "': " << e.what() << std::endl;
			}
		}
	}
};

Audio::Audio(): self(new Impl) {}
Audio::~Audio() {}

void Audio::reset() { self.reset(new Impl); };

bool Audio::isOpen() const {
	return !self->devices.empty();
}

void Audio::loadSample(std::string const& streamId, std::string const& filename) {
	boost::mutex::scoped_lock l(self->output.samples_mutex);
	self->output.samples.insert(streamId, std::auto_ptr<Sample>(new Sample(filename, getSR())));
}

void Audio::playSample(std::string const& streamId) {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	Command cmd = { Command::SAMPLE_RESET, streamId, 0.0 };
	o.commands.push_back(cmd);
}

void Audio::unloadSample(std::string const& streamId) {
	boost::mutex::scoped_lock l(self->output.samples_mutex);
	self->output.samples.erase(streamId);
}

void Audio::playMusic(std::map<std::string,std::string> const& filenames, bool preview, double fadeTime, double startPos) {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	o.disposing.clear();  // Delete disposed streams
	o.preloading.reset(new Music(filenames, getSR(), preview));
	Music& m = *o.preloading.get();
	m.seek(startPos);
	m.fadeRate = 1.0 / getSR() / fadeTime;
	o.commands.clear();  // Remove old unprocessed commands (they should not apply to the new music)
}

void Audio::playMusic(std::string const& filename, bool preview, double fadeTime, double startPos) {
	std::map<std::string,std::string> m;
	m["MAIN"] = filename;
	playMusic(m, preview, fadeTime, startPos);
}

void Audio::stopMusic() {
	std::map<std::string,std::string> m;
	playMusic(m, false, 0.0);
	{
		Output& o = self->output;
		// stop synth when music is stopped
		boost::mutex::scoped_lock l(o.synth_mutex);
		o.synth.reset();
	}
}

void Audio::fadeout(double fadeTime) {
	std::map<std::string,std::string> m;
	playMusic(m, false, fadeTime);
	{
		Output& o = self->output;
		// stop synth when music is stopped
		boost::mutex::scoped_lock l(o.synth_mutex);
		o.synth.reset();
	}
}

double Audio::getPosition() const {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	return (o.playing.empty() || o.preloading.get()) ? getNaN() : o.playing[0].pos();
}

double Audio::getLength() const {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	return (o.playing.empty() || o.preloading.get()) ? getNaN() : o.playing[0].duration();
}

bool Audio::isPlaying() const {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	return o.preloading.get() || !o.playing.empty();
}

void Audio::seek(double offset) {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	for(boost::ptr_vector<Music>::iterator it = o.playing.begin() ; it != o.playing.end() ; ++it) {
		it->seek(clamp(it->pos() + offset, 0.0, it->duration()));
	}
	pause(false);
}

void Audio::seekPos(double pos) {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	for(boost::ptr_vector<Music>::iterator it = o.playing.begin() ; it != o.playing.end() ; ++it) {
		it->seek(pos);
	}
	pause(false);
}

void Audio::pause(bool state) { self->output.paused = state; }
bool Audio::isPaused() const { return self->output.paused; }

void Audio::streamFade(std::string track, double fadeLevel) {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	Command cmd = { Command::TRACK_FADE, track, fadeLevel };
	o.commands.push_back(cmd);
}

void Audio::streamBend(std::string track, double pitchFactor) {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	Command cmd = { Command::TRACK_PITCHBEND, track, pitchFactor };
	o.commands.push_back(cmd);
}

void Audio::toggleSynth(Notes const& notes) {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.synth_mutex);
	o.synth.get() ?  o.synth.reset() : o.synth.reset(new Synth(notes, getSR()));
}

boost::ptr_vector<Analyzer>& Audio::analyzers() { return self->analyzers; }

boost::ptr_vector<Device>& Audio::devices() { return self->devices; }
