#include "audio.hh"

#include "configuration.hh"
#include "util.hh"
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/thread/mutex.hpp>
#include <libda/fft.hpp>  // For M_PI
#include <libda/portaudio.hpp>
#include <cmath>
#include <iostream>

class Music {
	struct Track {
		FFmpeg mpeg;
		float fadeLevel;
		Track(std::string const& filename, unsigned int sr): mpeg(false, true, filename, sr), fadeLevel(1.0f) {}
	};
	typedef boost::ptr_map<std::string, Track> Tracks;
	Tracks tracks; ///< Audio decoders
	double srate; ///< Sample rate
	int64_t m_pos; ///< Current sample position
	bool m_preview;
public:
	double fadeLevel;
	double fadeRate;
	typedef std::map<std::string,std::string> Files;
	Music(Files const& filenames, unsigned int sr, bool preview): srate(sr), m_pos(), m_preview(preview), fadeLevel(), fadeRate() {
		for (Files::const_iterator it = filenames.begin(), end = filenames.end(); it != end; ++it) {
			tracks.insert(it->first, std::auto_ptr<Track>(new Track(it->second, sr)));
		}
	}
	/// Sums the stream to output sample range, returns true if the stream still has audio left afterwards.
	bool operator()(float* begin, float* end) {
		bool eof = true;
		std::vector<float> mixbuf(end - begin);
		for (Tracks::iterator it = tracks.begin(), itend = tracks.end(); it != itend; ++it) {
			Track& t = *it->second;
			if (t.mpeg.audioQueue(&*mixbuf.begin(), &*mixbuf.end(), m_pos, t.fadeLevel)) eof = false;
		}
		m_pos += end - begin;
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
	/// Get current position in seconds
	double pos() const { return double(m_pos) / srate / 2.0; }
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
	enum { TRACK_FADE, SAMPLE_RESET } type;
	std::string track;
	double fadeLevel;
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
				if (!playing.empty()) playing[0].trackFade(cmd.track, cmd.fadeLevel);
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

struct Device {
	// Init
	const unsigned int in, out;
	const double rate;
	const std::string dev;
	portaudio::Stream stream;
	std::vector<Analyzer*> mics;
	Output* outptr;

	Device(unsigned int in, unsigned int out, double rate, std::string dev):
	  in(in), out(out), rate(rate), dev(dev),
	  stream(*this, in ? portaudio::Params().channelCount(in).device(dev, true) : (const PaStreamParameters*)NULL, (out ? portaudio::Params().channelCount(out).device(dev, false) : (const PaStreamParameters*)NULL), rate),
	  outptr()
	{
		mics.resize(in);
	}

	void start() {
		PaError err = Pa_StartStream(stream);
		if (err != paNoError) throw std::runtime_error("Cannot start PortAudio audio stream " + dev + ": " + Pa_GetErrorText(err));
	}

	int operator()(void const* input, void* output, unsigned long frames, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags) try {
		float const* in = static_cast<float const*>(input);
		float* out = static_cast<float*>(output);
		for (std::size_t i = 0; i < mics.size(); ++i) {
			if (!mics[i]) continue;  // No analyzer? -> Channel not used
			mics[i]->input(in, in + 1 * frames); // FIXME: needs libda iterators for multiple channel support
		}
		if (outptr) outptr->callback(out, out + 2 * frames);
		return paContinue;
	} catch (std::exception& e) {
		std::cerr << "Exception in audio callback: " << e.what() << std::endl;
		return paAbort;
	}
};

struct Audio::Impl {
	Output output;
	portaudio::Init init;
	boost::ptr_vector<Device> devices;
	boost::ptr_vector<Analyzer> analyzers;
	bool playback;
	Impl(): playback() {
		// Parse audio devices from config
		ConfigItem::StringList devs = config["audio/devices"].sl();
		for (ConfigItem::StringList::const_iterator it = devs.begin(), end = devs.end(); it != end; ++it) {
			try {
				struct Params {
					int in, out;
					unsigned int rate;
					std::string dev;
					std::vector<int> mics;
				} params = Params();
				params.rate = 48000;
				// Break into tokens:
				std::istringstream iss(*it);
				for (std::string token; std::getline(iss, token, ' '); ) {
					// Parse key=value
					std::istringstream iss2(token);
					std::string key;
					std::getline(iss2, key, '=');
					if (key == "out") iss2 >> params.out;
					else if (key == "in") iss2 >> params.in;
					else if (key == "rate") iss2 >> params.rate;
					else if (key == "dev") std::getline(iss2, params.dev);
					else if (key == "mics") {
						// Parse a comma-separated list of mics
						for (std::string mic; std::getline(iss2, mic, ','); ) {
							params.mics.push_back(0); // TODO/FIXME: implement
						}
						
					}
					else throw std::runtime_error("Unknown device parameter " + key);
					if (!iss2.eof()) throw std::runtime_error("Syntax error parsing device parameter " + key);
				}
				devices.push_back(new Device(params.in, params.out, params.rate, params.dev));
				Device& d = devices.back();
				// Assign mics for all channels of the device (TODO: proper assignments and limit the number of mics)
				for (unsigned int i = 0; i < d.in; ++i) {
					Analyzer* a = new Analyzer(d.rate);
					analyzers.push_back(a);
					d.mics[i] = a;
				}
				// Assign playback output for the first available stereo output
				if (!playback && d.out == 2) {
					d.outptr = &output;
					playback = true;
				}
				// Start capture/playback on this device
				d.start();
			} catch(std::runtime_error& e) {
				std::cerr << "Audio device '" << *it << "': " << e.what() << std::endl;
			}
		}
	}
};

Audio::Audio(): self(new Impl) {}
Audio::~Audio() {}

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
}

void Audio::fadeout(double fadeTime) {
	std::map<std::string,std::string> m;
	playMusic(m, false, fadeTime);
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

void Audio::toggleSynth(Notes const& notes) {
	Output& o = self->output;
	boost::mutex::scoped_lock l(o.synth_mutex);
	o.synth.get() ?  o.synth.reset() : o.synth.reset(new Synth(notes, getSR()));
}

boost::ptr_vector<Analyzer>& Audio::analyzers() { return self->analyzers; }

