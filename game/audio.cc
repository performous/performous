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
			begin[i] += mixbuf[i] * fadeLevel;
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
			begin[i] += mixbuf[i];
		}
		m_pos += end - begin;
	}
	void reset() {
		eof = false;
		m_pos = 0;
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
	}
};

struct Device {
	// Init
	unsigned int in, out;
	double rate;
	std::string dev;
	portaudio::Stream stream;
	std::vector<Analyzer*> mics;
	Output* outptr;

	Device(unsigned int in, unsigned int out, double rate, std::string dev):
	  in(in), out(out), rate(rate), dev(dev),
	  stream(*this, in ? portaudio::Params().channelCount(in).device(dev, true) : (const PaStreamParameters*)NULL, (out ? portaudio::Params().channelCount(out).device(dev, false) : (const PaStreamParameters*)NULL), rate)
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
			if (!mics[i]) continue;  // Channel not used
			mics[i]->input(in, in + 1 * frames);
		}
		if (outptr) outptr->callback(out, out + 2 * frames);
		return paContinue;
	} catch (std::exception& e) {
		std::cerr << "Exception in audio callback: " << e.what() << std::endl;
		return paAbort;
	}
};

struct Audio::Impl {
	portaudio::Init init;
	boost::ptr_vector<Device> devices;
	boost::ptr_vector<Analyzer> analyzers;
	Output output;
	bool playback;
	Impl() {
		devices.push_back(new Device(1, 2, 48000.0, ""));
		// One analyzer for mono input (TODO: fix callbackInput to allow more)
		analyzers.push_back(new Analyzer(48000.0));
		devices[0].mics[0] = &analyzers[0];
		playback = false;
		for (size_t i = 0; i < devices.size(); ++i) {
			Device& d = devices[i];
			// Assign playback output for the first available stereo output
			if (!playback && d.out == 2) {
				d.outptr = &output;
				playback = true;
			}
			// Start capture/playback on this device
			d.start();
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

boost::ptr_vector<Analyzer>& Audio::analyzers() { return self->analyzers; }

