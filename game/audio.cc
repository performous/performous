#include "audio.hh"

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <iostream>
#include <map>
#include "configuration.hh"
#include "libda/portaudio.hpp"
#include "util.hh"

extern const double m_pi;
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
		constexpr double maxError = 0.1;  // Step the clock instead of skewing if over 100 ms off
		// Full correction requires locking, but we can update max without lock too
		boost::mutex::scoped_try_lock l(m_mutex, boost::defer_lock);
		double max = getSeconds(audioPos + length);
		if (!l.try_lock()) {
			if (max > m_max) m_max = max; // Allow increasing m_max
			return;
		}
		// Mutex locked - do a full update
		ptime now = getTime();
		const double sys = pos_internal(now);  // Current position (based on system clock + corrections)
		const double audio = getSeconds(audioPos);  // Audio time
		const double diff = audio - sys;
		// Skew-based correction only if going forward and relatively well synced
		if (max > m_max && std::abs(diff) < maxError) {
			constexpr double fudgeFactor = 0.001;  // Adjustment ratio
			// Update base position (this should not affect the clock)
			m_baseTime = now;
			m_basePos = sys;
			// Apply a VERY ARTIFICIAL correction for clock!
			const double valadj = getSeconds(length) * 0.1 * rand() / RAND_MAX;  // Dither
			m_skew += (diff < valadj ? -1.0 : 1.0) * fudgeFactor;
			// Limits to keep things sane in abnormal situations
			m_skew = clamp(m_skew, -0.01, 0.01);
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
        Track(fs::path const& filename, unsigned int sr): mpeg(filename, sr), fadeLevel(1.0f), pitchFactor(0.0f) {}
	};
	typedef boost::ptr_map<std::string, Track> Tracks;
	Tracks tracks; ///< Audio decoders
	double srate; ///< Sample rate
	int64_t m_pos; ///< Current sample position
	bool m_preview;
	AudioClock m_clock;
	time_duration durationOf(int64_t samples) const { return microseconds(1e6 * samples / srate / 2.0); }
public:
	bool suppressCenterChannel;
	double fadeLevel;
	double fadeRate;
	typedef std::vector<float> Buffer;
	Music(Audio::Files const& files, unsigned int sr, bool preview):
	  srate(sr), m_pos(), m_preview(preview), fadeLevel(), fadeRate()
	{
		for (auto const& tf /* trackname-filename pair */: files) {
			if (tf.second.empty()) continue; // Skip tracks with no filenames; FIXME: Why do we even have those here, shouldn't they be eliminated earlier?
			tracks.insert(tf.first, std::auto_ptr<Track>(new Track(tf.second, sr)));
		}
		suppressCenterChannel = config["audio/suppress_center_channel"].b();
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
		// suppress center channel vocals
		if(suppressCenterChannel && !m_preview) {
			float diffLR;
			for (size_t i=0; i<mixbuf.size(); i+=2) {
				diffLR = begin[i] - begin[i+1];
				begin[i] = diffLR;
				begin[i+1] = diffLR;
			}
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
			FFmpeg& mpeg = it->second->mpeg;
			if (mpeg.terminating()) continue;  // Song loading failed or other error, won't ever get ready
			if (mpeg.audioQueue.prepare(m_pos)) continue;  // Buffering done
			ready = false;  // Need to wait for buffering
			break;
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
	double m_pos;
	FFmpeg mpeg;
	bool eof;
  public:
	Sample(fs::path const& filename, unsigned sr) : m_pos(), mpeg(filename, sr), eof(true) { }
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
	Synth(Notes const& notes, unsigned int sr) : m_notes(notes), srate(sr) {}
	void operator()(float* begin, float* end, double position) {
		static double phase = 0.0;
		for (float *i = begin; i < end; ++i) *i *= 0.3; // Decrease music volume

		std::vector<float> mixbuf(end - begin);
		Notes::const_iterator it = m_notes.begin();

		while (it != m_notes.end() && it->end < position) ++it;
		if (it == m_notes.end() || it->type == Note::SLEEP || it->begin > position) { phase = 0.0; return; }
		int note = it->note % 12;
		double d = (note + 1) / 13.0;
		double freq = MusicalScale().setNote(note + 4 * 12).getFreq();
		double value = 0.0;
		// Synthesize tones
		for (size_t i = 0, iend = mixbuf.size(); i != iend; ++i) {
			if (i % 2 == 0) {
				value = d * 0.2 * std::sin(phase) + 0.2 * std::sin(2 * phase) + (1.0 - d) * 0.2 * std::sin(4 * phase);
				phase += 2.0 * m_pi * freq / srate;
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

/// Audio output callback wrapper. The playback Device calls this when it needs samples.
struct Output {
	boost::mutex mutex;
	boost::mutex samples_mutex;
	boost::mutex synth_mutex;
	std::auto_ptr<Synth> synth;
	std::auto_ptr<Music> preloading;
	boost::ptr_vector<Music> playing, disposing;
	std::vector<Analyzer*> mics;  // Used for audio pass-through
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
		for (auto const& cmd: commands) {
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

	void callback(float* begin, float* end, double rate) {
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
		// Mix in microphones (if pass-through is enabled)
		if (mics.size() > 0 && config["audio/pass-through"].b()) {
			// Decrease music volume
			float amp = 1.0f / config["audio/pass-through_ratio"].f();
			if (amp != 1.0f) for (auto& s: boost::make_iterator_range(begin, end)) s *= amp;
			// Do the mixing
			for (auto& m: mics) if (m) m->output(begin, end, rate);
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
  stream(
    *this,
    portaudio::Params().channelCount(in).device(dev).suggestedLatency(config["audio/latency"].f()),
	portaudio::Params().channelCount(out).device(dev).suggestedLatency(config["audio/latency"].f()),
	rate),
  mics(in, nullptr),
  outptr()
{}

void Device::start() {
	PaError err = Pa_StartStream(stream);
	if (err != paNoError) throw std::runtime_error(std::string("Pa_StartStream: ") + Pa_GetErrorText(err));
}

int Device::operator()(void const* input, void* output, unsigned long frames, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags) try {
	float const* inbuf = static_cast<float const*>(input);
	float* outbuf = static_cast<float*>(output);
	for (std::size_t i = 0; i < mics.size(); ++i) {
		if (!mics[i]) continue;  // No analyzer? -> Channel not used
		da::sample_const_iterator it = da::sample_const_iterator(inbuf + i, in);
		mics[i]->input(it, it + frames);
	}
	if (outptr) outptr->callback(outbuf, outbuf + 2 * frames, rate);
	return paContinue;
} catch (std::exception& e) {
	std::cerr << "Exception in audio callback: " << e.what() << std::endl;
	return paAbort;
}

struct Audio::Impl {
	Output output;
	portaudio::Init init;
	boost::ptr_vector<Analyzer> analyzers;
	boost::ptr_vector<Device> devices;
	bool playback;
	std::string selectedBackend = Audio::backendConfig().getValue();
	Impl(): init(), playback() {
	populateBackends(portaudio::AudioBackends().getBackends());
	std::clog << portaudio::AudioBackends().dump() << std::flush; // Dump PortAudio backends and devices to log.
		// Parse audio devices from config
		ConfigItem::StringList devs = config["audio/devices"].sl();
		for (ConfigItem::StringList::const_iterator it = devs.begin(), end = devs.end(); it != end; ++it) {
			try {
				struct Params {
					unsigned out, in;
					unsigned int rate;
					std::string dev;
					std::vector<std::string> mics;
				} params = Params();
				params.out = 0;
				params.in = 0;
				params.rate = 48000;
				// Break into tokens:
				std::map<std::string, std::string> keyvalues = parseKeyValuePairs(*it);
				for (std::map<std::string, std::string>::const_iterator it2 = keyvalues.begin();
				  it2 != keyvalues.end(); ++it2) {
					// Handle keys
					std::string key = it2->first;
					std::istringstream iss(it2->second);
					if (key == "out") iss >> params.out;
					else if (key == "in") iss >> params.in;
					else if (key == "rate") iss >> params.rate;
					else if (key == "dev") std::getline(iss, params.dev);
					else if (key == "mics") {
						// Parse a comma-separated list of mics
						for (std::string mic; std::getline(iss, mic, ','); params.mics.push_back(mic)) {}
					}
					else throw std::runtime_error("Unknown device parameter " + key);
					if (!iss.eof()) throw std::runtime_error("Syntax error parsing device parameter " + key);
				}
				// Sync mics/in settings together
				if (params.in == 0) params.in = params.mics.size();
				else params.mics.resize(params.in);
				portaudio::AudioDevices ad(PaHostApiTypeId(PaHostApiNameToHostApiTypeId(selectedBackend)));
				auto const& info = ad.find(params.dev);
				std::clog << "audio/info: Trying audio device \"" << params.dev << "\", idx: " << info.idx
					<< ", in: " << params.in << ", out: " << params.out << std::endl;
				if (info.in < int(params.mics.size())) throw std::runtime_error("Device doesn't have enough input channels");
				if (info.out < int(params.out)) throw std::runtime_error("Device doesn't have enough output channels");
				// Match found if we got here, construct a device
				auto d = new Device(params.in, params.out, params.rate, info.idx);
				devices.push_back(d);
				// Start capture/playback on this device (likely to throw due to audio system errors)
				// NOTE: When it throws we want to keep the device in devices to avoid calling ~Device
				// which often would hit the Pa_CloseStream hang bug and terminate the application.
				d->start();
				// Assign mics for all channels of the device
				int assigned_mics = 0;
				for (unsigned int j = 0; j < params.in; ++j) {
					if (analyzers.size() >= AUDIO_MAX_ANALYZERS) break; // Too many mics
					std::string const& m = params.mics[j];
					if (m.empty()) continue; // Input channel not used
					// Check that the color is not already taken
					bool mic_used = false;
					for (size_t mi = 0; mi < analyzers.size(); ++mi) {
						if (analyzers[mi].getId() == m) { mic_used = true; break; }
					}
					if (mic_used) continue;
					// Add the new analyzer
					Analyzer* a = new Analyzer(d->rate, m);
					analyzers.push_back(a);
					d->mics[j] = a;
					++assigned_mics;
				}
				// Assign playback output for the first available stereo output
				if (!playback && d->out == 2) { d->outptr = &output; playback = true; }
				std::clog << "audio/info: Using audio device: " << info.desc();
				if (assigned_mics) std::clog << ", input channels: " << assigned_mics;
				if (params.out) std::clog << ", output channels: " << params.out;
				std::clog << std::endl;
			} catch(std::runtime_error& e) {
				std::clog << "audio/error: Audio device '" << *it << "': " << e.what() << std::endl;
			}
		}
		// Assign mic buffers to the output for pass-through
		for (size_t i = 0; i < analyzers.size(); ++i)
			output.mics.push_back(&analyzers[i]);
	}
};

Audio::Audio(): self(new Impl) {}
Audio::~Audio() {}

ConfigItem& Audio::backendConfig() {
	static ConfigItem& backend = config["audio/backend"];
	return backend;
}

void Audio::restart() { self.reset(new Impl); }
void Audio::close() { self.reset(); }

bool Audio::isOpen() const {
	return !self->devices.empty();
}

bool Audio::hasPlayback() const {
	for (size_t i = 0; i < self->devices.size(); ++i)
		if (self->devices[i].isOutput()) return true;
	return false;
}

void Audio::loadSample(std::string const& streamId, fs::path const& filename) {
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

void Audio::playMusic(Audio::Files const& filenames, bool preview, double fadeTime, double startPos) {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	o.disposing.clear();  // Delete disposed streams
	o.preloading.reset(new Music(filenames, getSR(), preview));
	Music& m = *o.preloading.get();
	m.seek(startPos);
	m.fadeRate = 1.0 / getSR() / fadeTime;
	o.commands.clear();  // Remove old unprocessed commands (they should not apply to the new music)
}

void Audio::playMusic(fs::path const& filename, bool preview, double fadeTime, double startPos) {
	Audio::Files m;
	m["MAIN"] = filename;
	playMusic(m, preview, fadeTime, startPos);
}

void Audio::stopMusic() {
	playMusic(Audio::Files(), false, 0.0);
	{
		Output& o = self->output;
		// stop synth when music is stopped
		boost::mutex::scoped_lock l(o.synth_mutex);
		o.synth.reset();
	}
}

void Audio::fadeout(double fadeTime) {
	playMusic(Audio::Files(), false, fadeTime);
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
	for (auto& trk: o.playing) trk.seek(clamp(trk.pos() + offset, 0.0, trk.duration()));
	pause(false);
}

void Audio::seekPos(double pos) {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	for (auto& trk: o.playing) trk.seek(pos);
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

void Audio::toggleCenterChannelSuppressor() {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.mutex);
	for (size_t i = 0; i < o.playing.size(); i++) {
		o.playing[i].suppressCenterChannel = !o.playing[i].suppressCenterChannel;
	}
}

boost::ptr_vector<Analyzer>& Audio::analyzers() { return self->analyzers; }

boost::ptr_vector<Device>& Audio::devices() { return self->devices; }
