#include "audio.hh"

#include "chrono.hh"
#include "configuration.hh"
#include "libda/portaudio.hpp"
#include "screen_songs.hh"
#include "game.hh"
#include "analyzer.hh"
#include "songs.hh"
#include "util.hh"

#include "aubio/aubio.h"
#include <boost/range/iterator_range.hpp>

#include <cmath>
#include <future>
#include <iostream>
#include <map>
#include <unordered_map>

int PaHostApiNameToHostApiTypeId (const std::string& name) {
	if (name == "Auto") return 1337;
	if (name == "Windows DirectSound") return 1;
	if (name == "MME") return 2;
	if (name == "ASIO") return 3;
	if (name == "Core Audio" || name == "CoreAudio") return 5;
	if (name == "OSS") return 7; // Not an error, stupid PortAudio.
	if (name == "ALSA") return 8;
	if (name == "Windows WDM-KS") return 11;
	if (name == "JACK Audio Connection Kit") return 12;
	if (name == "Windows WASAPI") return 13;
	throw std::runtime_error("Invalid PortAudio HostApiTypeId Specified.");
}

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

void AudioClock::timeSync(Seconds audioPos, Seconds length) {
	constexpr Seconds maxError = 100ms;  // Step the clock instead of skewing if over 100 ms off
	// Full correction requires locking, but we can update max without lock too
	std::unique_lock<std::mutex> l(m_mutex, std::defer_lock);
	Seconds max = audioPos + length;
	if (!l.try_lock()) {
		if (max > m_max.load()) m_max = max; // Allow increasing m_max
		return;
	}
	// Mutex locked - do a full update
	auto now = Clock::now();
	const Seconds sys = pos_internal(now);  // Current position (based on system clock + corrections)
	const Seconds audio = audioPos;  // Audio time
	const Seconds diff = audio - sys;
	// Skew-based correction only if going forward and relatively well synced
	if (max > m_max.load() && std::abs(diff.count()) < maxError.count()) {
		constexpr double fudgeFactor = 0.001;  // Adjustment ratio
		// Update base position (this should not affect the clock)
		m_baseTime = now;
		m_basePos = sys;
		// Apply a VERY ARTIFICIAL correction for clock!
		const Seconds valadj = length * 0.1 * rand() / RAND_MAX;  // Dither
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

Seconds AudioClock::pos_internal(Time now) const {
	Seconds t = m_basePos + (1.0 + m_skew) * (now - m_baseTime);
	return std::min<Seconds>(t, m_max);
}

Seconds AudioClock::pos() const {
	std::lock_guard<std::mutex> l(m_mutex);
	return pos_internal(Clock::now());
}

Music::Music(Game& game, Audio::Files const& files, unsigned int sr, bool preview)
: srate(sr), m_preview(preview), m_game(game) {
	for (auto const& tf /* trackname-filename pair */: files) {
		if (tf.second.empty()) continue; // Skip tracks with no filenames; FIXME: Why do we even have those here, shouldn't they be eliminated earlier?
		tracks.emplace(tf.first, std::make_unique<Track>(tf.second, sr));
	}
	suppressCenterChannel = config["audio/suppress_center_channel"].b();
}

unsigned Audio::aubio_win_size = 1536;
unsigned Audio::aubio_hop_size = 768;

std::unique_ptr<aubio_tempo_t, void(*)(aubio_tempo_t*)> Audio::aubioTempo =
					std::unique_ptr<aubio_tempo_t, void(*)(aubio_tempo_t*)>(
						new_aubio_tempo(
							"default",
							Audio::aubio_win_size,
							Audio::aubio_hop_size,
							static_cast<uint_t>(Audio::getSR())),[](aubio_tempo_t* p) {
							if (p != nullptr) {
								del_aubio_tempo(p);
								}
							}
					);

std::recursive_mutex Audio::aubio_mutex;

bool Music::operator()(float* begin, float* end) {
	std::int64_t samples = end - begin;
	m_clock.timeSync(durationOf(m_pos), durationOf(samples)); // Keep the clock synced
	bool eof = true;
	Buffer mixbuf(static_cast<size_t>(samples));
	for (auto& kv: tracks) {
		Track& t = *kv.second;
// #if 0 // FIXME: Include this code bit once there is a sane pitch shifting algorithm
// //            if (it->first == "guitar") std::cout << t.pitchFactor << std::endl;
//             if (t.pitchFactor != 0) { // Pitch shift
//                 Buffer tempbuf(end - begin);
//                 // Get audio to temp buffer
//                 if (t.mpeg.audioQueue(tempbuf.data(), tempbuf.data() + tempbuf.size(), m_pos, t.fadeLevel)) eof = false;
//                 // Do the magic
//                 PitchShift(tempbuf.begin(), tempbuf.end(), t.pitchFactor);
//                 // Mix with other tracks
//                 Buffer::iterator m = mixbuf.begin();
//                 Buffer::iterator b = tempbuf.begin();
//                 while (b != tempbuf.end())
//                     *m++ += (*b++);
//             // Otherwise just get the audio and mix it straight away
//             } else
// #endif
		if (t.audioBuffer.read(mixbuf.data(), static_cast<std::int64_t>(mixbuf.size()), m_pos, static_cast<float>(t.fadeLevel))) eof = false;
	}
	m_pos += samples;
	const float volume = static_cast<float>(m_preview ? config["audio/preview_volume"].ui() : config["audio/music_volume"].ui()) / 100.0f;
	for (size_t i = 0, iend = mixbuf.size(); i != iend; ++i) {
		if (i % 2 == 0) {
			fadeLevel += fadeRate;
			if (fadeLevel <= 0.0f) return false;
			if (fadeLevel > 1.0f) { fadeLevel = 1.0f; fadeRate = 0.0f; }
		}
		begin[i] += static_cast<float>(mixbuf[i] * fadeLevel * volume);
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

double Music::duration() const {
	double dur = 0.0;
	for (auto& kv: tracks) dur = std::max(dur, kv.second->audioBuffer.duration());
	return dur;
}

bool Music::prepare() {
	bool ready = true;
	for (auto& kv: tracks) {
		auto& audioBuffer = kv.second->audioBuffer;
		if (audioBuffer.prepare(m_pos)) {
			if (kv.first == "background" && m_preview && m_pos > 0) {
				auto previewSamples = audioBuffer.makePreviewBuffer();
				fvec_t* previewBeats = ScreenSongs::previewBeatsBuffer.get();
				intptr_t readptr = 0;
				fvec_t* tempoSamplePtr = new_fvec(Audio::aubio_hop_size);
				std::lock_guard<std::recursive_mutex> l(Audio::aubio_mutex);
				ScreenSongs* sSongs = dynamic_cast<ScreenSongs*>(m_game.getScreen("Songs"));
				if (!sSongs->getSongs().currentPtr()) return false;
				double pstart = sSongs->getSongs().currentPtr()->preview_start;
				pstart = (std::isnan(pstart) ? 0.0 : pstart);
				double first_period = 0.0, first_beat = 0.0;
				std::vector<double> extra_beats;
				Song::Beats& beats = sSongs->getSongs().currentPtr()->beats;
				if (!sSongs->getSongs().currentPtr()->hasControllers()) {
				while ((readptr + Audio::aubio_hop_size) <= previewSamples->length) {
					tempoSamplePtr->data = &previewSamples->data[readptr];
					aubio_tempo_do(Audio::aubioTempo.get(),tempoSamplePtr,previewBeats);
					if (previewBeats->data[0] != 0) {
						double beatSecs = aubio_tempo_get_last_s(Audio::aubioTempo.get());
							if (beats.empty()) { // Store time and period of first detected beat.
								first_beat = beatSecs;
								first_period = aubio_tempo_get_period_s(Audio::aubioTempo.get());
							}
						beats.push_back(beatSecs + pstart);
					}
					readptr += Audio::aubio_hop_size;
				}
					if (!beats.empty()) {
						double newBeat = first_beat - first_period;
						while (newBeat > 0.02) {
							extra_beats.push_back(newBeat + pstart);
							newBeat -= first_period;
						}
						beats.insert(beats.begin(),extra_beats.rbegin(),extra_beats.rend());
					}
				}
			}
		continue;  // Buffering done
		}
		ready = false;  // Need to wait for buffering
		break;
	}
	return ready;
}

void Music::trackFade(std::string const& name, double fadeLevel) {
	auto it = tracks.find(name);
	if (it == tracks.end()) return;
	it->second->fadeLevel = fadeLevel;
}

void Music::trackPitchBend(std::string const& name, double pitchFactor) {
	auto it = tracks.find(name);
	if (it == tracks.end()) return;
	it->second->pitchFactor = pitchFactor;
}

struct Sample {
  private:
	std::int64_t m_pos;
	AudioBuffer audioBuffer;
	bool eof;
  public:
	Sample(fs::path const& filename, unsigned sr) : m_pos(), audioBuffer(filename, sr), eof(true) { }
	void operator()(float* begin, float* end) {
		if(eof) {
			// No more data to play in this sample
			return;
		}
		if (end <= begin) {
			eof = true;
			return;
		}
		std::int64_t size = end - begin;
		std::vector<float> mixbuf(static_cast<size_t>(size));
		if(!audioBuffer.read(mixbuf.data(), size, m_pos, 1.0)) {
			eof = true;
		}
		const auto failVolume = static_cast<float>(config["audio/fail_volume"].ui()) / 100.0f;
		for (size_t i = 0, iend = mixbuf.size(); i != iend; ++i) {
			begin[i] += mixbuf[i] * failVolume;
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
		for (float *i = begin; i < end; ++i) *i *= 0.3f; // Decrease music volume
		std::int64_t size = end - begin;
		if (end <= begin) return;
		std::vector<float> mixbuf(static_cast<size_t>(size));
		Notes::const_iterator it = m_notes.begin();

		while (it != m_notes.end() && it->end < position) ++it;
		if (it == m_notes.end() || it->type == Note::Type::SLEEP || it->begin > position) { phase = 0.0; return; }
		float note = std::fmod(it->note, 12.0f);
		double d = (note + 1.0) / 13.0;
		double freq = MusicalScale().setNote(note + 4.0 * 12.0).getFreq();
		double value = 0.0;
		// Synthesize tones
		for (size_t i = 0, iend = mixbuf.size(); i != iend; ++i) {
			if (i % 2 == 0) {
				value = d * 0.2 * std::sin(phase) + 0.2 * std::sin(2 * phase) + (1.0 - d) * 0.2 * std::sin(4 * phase);
				phase += TAU * freq / srate;
			}
			begin[i] += static_cast<float>(value);
		}
	}
};

struct Command {
	enum class Type { TRACK_FADE, TRACK_PITCHBEND, SAMPLE_RESET } type;
	std::string track;
	double factor;
};

/// Audio output callback wrapper. The playback Device calls this when it needs samples.
struct Output {
	std::mutex mutex;
	std::mutex samples_mutex;
	std::mutex synth_mutex;
	std::unique_ptr<Synth> synth;
	std::unique_ptr<Music> preloading;
	std::vector<std::unique_ptr<Music>> playing, disposing;
	std::vector<Analyzer*> mics;  // Used for audio pass-through
	std::unordered_map<std::string, std::unique_ptr<Sample>> samples;
	std::vector<Command> commands;
	std::atomic<bool> paused{ false };
	Output(): paused(false) {}

	void callbackUpdate() {
		std::unique_lock<std::mutex> l(mutex, std::try_to_lock);
		if (!l.owns_lock()) return;  // No update now, try again later (cannot stop and wait for mutex to be released)
		// Move from preloading to playing, if ready
		if (preloading && preloading->prepare()) {
			std::clog << "audio/debug: preload done -> playing " << preloading.get() << std::endl;
			if (!playing.empty()) playing[0]->fadeRate = -preloading->fadeRate;  // Fade out the old music
			playing.insert(playing.begin(), std::move(preloading));
		}
		// Process commands
		for (auto const& cmd: commands) {
			switch (cmd.type) {
			case Command::Type::TRACK_FADE:
				if (!playing.empty()) playing[0]->trackFade(cmd.track, cmd.factor);
				break;
			case Command::Type::TRACK_PITCHBEND:
				if (!playing.empty()) playing[0]->trackPitchBend(cmd.track, cmd.factor);
				break;
			case Command::Type::SAMPLE_RESET:
				auto it = samples.find(cmd.track);
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
		auto arrayEnd = playing.end();
		for (auto i = playing.begin(); i != arrayEnd;) {
			bool keep = (*i->get())(begin, end);  // Do the actual mixing
			std::unique_lock<std::mutex> l(mutex, std::defer_lock);
			if (!keep && l.try_lock()) {
				// Dispose streams no longer needed by moving them to another container (that will be cleared by another thread).
				disposing.push_back(std::move(*i));
				i = playing.erase(i);
				arrayEnd = playing.end();
			}
			else { ++i; }
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
			std::unique_lock<std::mutex> l(samples_mutex, std::defer_lock);
			if(l.try_lock()) {
				for(auto it = samples.begin() ; it != samples.end() ; ++it) {
					(*it->second)(begin, end);
				}
			}
		}
		// Mix synth if available (should be done at the end)
		{
			std::unique_lock<std::mutex> l(synth_mutex, std::defer_lock);
			if(l.try_lock() && synth.get() && !playing.empty()) {
				(*synth.get())(begin, end, playing[0]->pos());
			}
		}
	}
};


Device::Device(int in, int out, double rate, PaDeviceIndex dev):
  in(in), out(out), rate(rate), dev(dev),
  stream(*this,
  portaudio::Params().channelCount(in).device(dev).suggestedLatency(config["audio/latency"].f()),
  portaudio::Params().channelCount(out).device(dev).suggestedLatency(config["audio/latency"].f()), rate),
  mics(static_cast<size_t>(in), nullptr),
  outptr()
{}

void Device::start() {
	PaError err = Pa_StartStream(stream);
	if (err != paNoError) throw std::runtime_error(std::string("Pa_StartStream: ") + Pa_GetErrorText(err));
}

void Device::stop() {
	PORTAUDIO_CHECKED(Pa_AbortStream, (stream));
}

bool Device::isChannel(std::string const& name) const {
	if (name == "OUT")
		return isOutput();
	for (auto const& m: mics)
		if (m && m->getId() == name)
			return true;
	return false;
}

int Device::operator()(float const* inbuf, float* outbuf, std::ptrdiff_t frames) try {
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
	std::deque<Analyzer> analyzers;
	std::deque<Device> devices;
	bool playback = false;
	std::string selectedBackend = Audio::backendConfig().getValue();
	Impl() {
		std::clog << portaudio::AudioBackends().dump() << std::flush; // Dump PortAudio backends and devices to log.
		// Parse audio devices from config
		ConfigItem::StringList devs = config["audio/devices"].sl();
		for (ConfigItem::StringList::const_iterator it = devs.begin(), end = devs.end(); it != end; ++it) {
			try {
				struct Params {
					int out, in;
					unsigned int rate;
					std::string dev;
					std::vector<std::string> mics;
				} params = Params();
				params.out = 0;
				params.in = 0;
				params.rate = 48000u;
				// Break into tokens:
				for (auto& kv: parseKeyValuePairs(*it)) {
					// Handle keys
					std::string key = kv.first;
					std::istringstream iss(kv.second);
					if (key == "out") iss >> params.out;
					else if (key == "in") iss >> params.in;
					else if (key == "rate") iss >> params.rate;
					else if (key == "dev") std::getline(iss, params.dev);
					else if (key == "mics") {
						// Parse a comma-separated list of mics
						for (std::string mic; std::getline(iss, mic, ','); params.mics.push_back(mic)) {
							++params.in;
						}
					}
					else throw std::runtime_error("Unknown device parameter " + key);
					if (!iss.eof()) throw std::runtime_error("Syntax error parsing device parameter " + key);
				}
				if (params.mics.size() < static_cast<size_t>(params.in)) { params.mics.resize(static_cast<size_t>(params.in)); }
				portaudio::AudioDevices ad(PaHostApiTypeId(PaHostApiNameToHostApiTypeId(selectedBackend)));
					bool wantOutput = (params.in == 0) ? true : false;
					int num;
					std::string msg = "audio/info: Device string empty; will look for a device with at least ";
					if (wantOutput) {
						msg += std::to_string(params.out) + " output channels.";
						num = params.out;
					}
					else {
						msg += std::to_string(params.in) + " input channels.";
						num = params.in;
					}
					if (!params.dev.empty()) {
					std::clog << "audio/debug: Will try to find device matching dev: " << params.dev << std::endl;
					}
					else { std::clog << msg << std::endl; }
					portaudio::DeviceInfo const& info = ad.find(params.dev, wantOutput, num);
					std::clog << "audio/info: Found: " << info.name << ", in: " << info.in << ", out: " << info.out << std::endl;
				if (info.in < static_cast<int>(params.mics.size())) throw std::runtime_error("Device doesn't have enough input channels");
				if (info.out < params.out) throw std::runtime_error("Device doesn't have enough output channels");
				// Match found if we got here, construct a device
				devices.emplace_back(params.in, params.out, params.rate, info.index);
				Device& d = devices.back();
				// Assign mics for all channels of the device
				int assigned_mics = 0;
				for (unsigned j = 0; j < static_cast<unsigned>(params.in); ++j) {
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
					analyzers.emplace_back(d.rate, m);
					d.mics[j] = &analyzers.back();
					++assigned_mics;
				}
				// Assign playback output for the first available stereo output
				if (!playback && d.out == 2) { d.outptr = &output; playback = true; }
				std::clog << "audio/info: Using audio device: " << info.desc();
				if (assigned_mics) std::clog << ", input channels: " << assigned_mics;
				if (params.out) std::clog << ", output channels: " << params.out;
				std::clog << std::endl;
				// Start capture/playback on this device (likely to throw due to audio system errors)
				// NOTE: When it throws we want to keep the device in devices to avoid calling ~Device
				// which often would hit the Pa_CloseStream hang bug and terminate the application.
				d.start();
			} catch(std::runtime_error& e) {
				std::clog << "audio/error: Audio device '" << *it << "': " << e.what() << std::endl;
			}
		}
		// Assign mic buffers to the output for pass-through
		for (size_t i = 0; i < analyzers.size(); ++i)
			output.mics.push_back(&analyzers[i]);
	}
	~Impl() {
		// stop all audio streams befor destoying the object.
		// else portaudio will keep sending data to those destroyed
		// objects.
		for (auto& device: devices) try { device.stop(); } catch (const std::exception &e) { std::clog << "audio/error: " << e.what(); }
	}
};

portaudio::Init Audio::init;

Audio::Audio() {
	aubio_tempo_set_silence(Audio::aubioTempo.get(), -50.0f);
	aubio_tempo_set_threshold(Audio::aubioTempo.get(), 0.4f);
	populateBackends(portaudio::AudioBackends().getBackends());
	self = std::make_unique<Impl>();
}

Audio::~Audio() {
	close();
}

ConfigItem& Audio::backendConfig() {
	static ConfigItem& backend = config["audio/backend"];
	return backend;
}

void Audio::restart() {
	close();
	self = std::make_unique<Impl>();
}

void Audio::close() {
	self.reset();
}

bool Audio::isOpen() const {
	return !self->devices.empty();
}

bool Audio::hasPlayback() const {
	for (size_t i = 0; i < self->devices.size(); ++i)
		if (self->devices[i].isOutput()) return true;
	return false;
}

void Audio::loadSample(std::string const& streamId, fs::path const& filename) {
	std::lock_guard<std::mutex> l(self->output.samples_mutex);
	self->output.samples.emplace(streamId, std::unique_ptr<Sample>(new Sample(filename, static_cast<unsigned>(getSR()))));
}

void Audio::playSample(std::string const& streamId) {
	Output& o = self->output;
	std::lock_guard<std::mutex> l(o.mutex);
	Command cmd = { Command::Type::SAMPLE_RESET, streamId, 0.0 };
	o.commands.push_back(cmd);
}

void Audio::unloadSample(std::string const& streamId) {
	std::lock_guard<std::mutex> l(self->output.samples_mutex);
	self->output.samples.erase(streamId);
}

void Audio::playMusic(Game& game, Audio::Files const& filenames, bool preview, double fadeTime, double startPos) {
	Output& o = self->output;
	auto m = std::make_unique<Music>(game, filenames, getSR(), preview);
	m->seek(startPos);
	m->fadeRate = 1.0 / getSR() / fadeTime;
	// Format debug message
	std::string logmsg = "audio/debug: playMusic(";
	for (auto& kv: filenames) logmsg += kv.first + "=" + kv.second.filename().string() + ", ";
	logmsg += ") -> ";
	std::clog << logmsg << m.get() << std::endl;
	// Send to audio playback thread
	std::lock_guard<std::mutex> l(o.mutex);
	if (o.preloading) std::clog << "audio/debug: earlier music still preloading, disposing " << o.preloading.get() << std::endl;
	o.preloading = std::move(m);
	o.disposing.clear();  // Delete disposed streams
	o.commands.clear();  // Remove old unprocessed commands (they should not apply to the new music)
}

void Audio::playMusic(Game& game, fs::path const& filename, bool preview, double fadeTime, double startPos) {
	Audio::Files m;
	m["MAIN"] = filename;
	playMusic(game, m, preview, fadeTime, startPos);
}

void Audio::stopMusic(Game& game) {
	playMusic(game, Audio::Files(), false, 0.0);
	{
		Output& o = self->output;
		// stop synth when music is stopped
		std::lock_guard<std::mutex> l(o.synth_mutex);
		o.synth.reset();
	}
}

void Audio::fadeout(Game& game, double fadeTime) {
	playMusic(game, Audio::Files(), false, fadeTime);
	{
		Output& o = self->output;
		// stop synth when music is stopped
		std::lock_guard<std::mutex> l(o.synth_mutex);
		o.synth.reset();
	}
}

double Audio::getPosition() const {
	Output& o = self->output;
	std::lock_guard<std::mutex> l(o.mutex);
	return (o.playing.empty() || o.preloading.get()) ? getNaN() : o.playing[0]->pos();
}

double Audio::getLength() const {
	Output& o = self->output;
	std::lock_guard<std::mutex> l(o.mutex);
	return (o.playing.empty() || o.preloading.get()) ? getNaN() : o.playing[0]->duration();
}

bool Audio::isPlaying() const {
	Output& o = self->output;
	std::lock_guard<std::mutex> l(o.mutex);
	return o.preloading || !o.playing.empty();
}

void Audio::seek(double offset) {
	Output& o = self->output;
	std::lock_guard<std::mutex> l(o.mutex);
	for (auto& trk: o.playing) trk->seek(clamp(trk->pos() + offset, 0.0, trk->duration()));
	pause(false);
}

void Audio::seekPos(double pos) {
	Output& o = self->output;
	std::lock_guard<std::mutex> l(o.mutex);
	for (auto& trk: o.playing) trk->seek(pos);
	pause(false);
}

void Audio::pause(bool state) { self->output.paused = state; }
bool Audio::isPaused() const { return self->output.paused; }

void Audio::streamFade(std::string track, double fadeLevel) {
	Output& o = self->output;
	std::lock_guard<std::mutex> l(o.mutex);
	Command cmd = { Command::Type::TRACK_FADE, track, fadeLevel };
	o.commands.push_back(cmd);
}

void Audio::streamBend(std::string track, double pitchFactor) {
	Output& o = self->output;
	std::lock_guard<std::mutex> l(o.mutex);
	Command cmd = { Command::Type::TRACK_PITCHBEND, track, pitchFactor };
	o.commands.push_back(cmd);
}

void Audio::toggleSynth(Notes const& notes) {
	Output& o = self->output;
	std::lock_guard<std::mutex> l(o.synth_mutex);
	if (o.synth.get()) o.synth.reset();
	else o.synth = std::make_unique<Synth>(notes, getSR());
}

void Audio::toggleCenterChannelSuppressor() {
	Output& o = self->output;
	std::lock_guard<std::mutex> l(o.mutex);
	for (size_t i = 0; i < o.playing.size(); i++) {
		o.playing[i]->suppressCenterChannel = !o.playing[i]->suppressCenterChannel;
	}
}

std::deque<Analyzer>& Audio::analyzers() {
	return self->analyzers;
}

std::deque<Device>& Audio::devices() {
	return self->devices;
}
