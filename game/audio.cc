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
public:
	double fadeLevel;
	double fadeRate;
	typedef std::map<std::string,std::string> Files;
	Music(Files const& filenames, unsigned int sr): srate(sr), m_pos(), fadeLevel(), fadeRate() {
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

// rename to Sample once totally implemented
struct SampleNew {
  private:
	unsigned int srate;
	double m_pos;
	FFmpeg mpeg;
	bool eof;
  public:
	SampleNew(std::string filename, unsigned sr) : srate(sr), m_pos(), mpeg(false, true, filename, sr), eof(true) { }
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
	enum { TRACK_FADE } type;
	std::string track;
	double fadeLevel;
};

struct Audio::Impl {
	boost::mutex mutex;
	std::auto_ptr<Music> preloading;
	boost::ptr_vector<Music> playing, disposing;
	boost::ptr_map<std::string, SampleNew> samples;
	boost::ptr_vector<Analyzer> analyzers;
	std::vector<Command> commands;
	portaudio::Init init;
	boost::ptr_vector<portaudio::Stream> streams;
	volatile bool paused;
	Impl(): paused(false) {
		std::string dev;
		streams.push_back(new portaudio::Stream(*this, portaudio::Params().channelCount(1).device(dev, true), portaudio::Params().channelCount(2).device(dev, false), 48000));
		// One analyzer for mono input (TODO: fix callbackInput to allow more)
		analyzers.push_back(new Analyzer(48000.0));
		PaError err = Pa_StartStream(streams[0]);
		if( err != paNoError ) throw std::runtime_error("Cannot start PortAudio audio stream " + dev + ": " + Pa_GetErrorText(err));
	}
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
			}
		}
		commands.clear();
	}
	void callbackInput(float const* begin, float const* end) {
		analyzers[0].input(begin, end);  // TODO: non-pointer sample iterators and support more analyzers
	}
	void callbackOutput(float* begin, float* end) {
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
		for(boost::ptr_map<std::string, SampleNew>::iterator it = samples.begin() ; it != samples.end() ; ++it) {
			boost::mutex::scoped_try_lock l(mutex);
			(*it->second)(begin, end);
		}
	}
	int operator()(void const* input, void* output, unsigned long frames, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags) try {
		float const* in = static_cast<float const*>(input);
		float* out = static_cast<float*>(output);
		callbackInput(in, in + 1 * frames);  // TODO: no hardcoded channel count
		callbackUpdate();
		callbackOutput(out, out + 2 * frames);
		return paContinue;
	} catch (std::exception& e) {
		std::cerr << "Exception in audio callback: " << e.what() << std::endl;
		return paAbort;
	}
};

Audio::Audio(): self(new Impl) {}
Audio::~Audio() {}

bool Audio::isOpen() const {
	boost::mutex::scoped_lock l(self->mutex);
	return !self->streams.empty();
}

void Audio::loadSample(std::string streamId, std::string filename) {
	{
		boost::mutex::scoped_lock l(self->mutex);
		self->samples.insert(streamId, new SampleNew(filename, getSR()));
	}
}
void Audio::playSample(std::string streamId) {
	boost::mutex::scoped_lock l(self->mutex);
	boost::ptr_map<std::string, SampleNew>::iterator it = self->samples.find(streamId);
	if( it == self->samples.end() ) {
		throw std::runtime_error("Cannot play sample : " + streamId);
	}
	it->second->reset();
}

void Audio::unloadSample(std::string streamId) {
	{
		boost::mutex::scoped_lock l(self->mutex);
		self->samples.erase(streamId);
	}
}

void Audio::playMusic(std::map<std::string,std::string> const& filenames, bool preview, double fadeTime, double startPos) {
	boost::mutex::scoped_lock l(self->mutex);
	self->disposing.clear();  // Delete disposed streams
	self->preloading.reset(new Music(filenames, getSR()));
	Music& m = *self->preloading.get();
	m.seek(startPos);
	m.fadeRate = 1.0 / getSR() / fadeTime;
	self->commands.clear();  // Remove old unprocessed commands (they should not apply to the new music)
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
	boost::mutex::scoped_lock l(self->mutex);
	return (self->playing.empty() || self->preloading.get()) ? getNaN() : self->playing[0].pos();
}

double Audio::getLength() const {
	boost::mutex::scoped_lock l(self->mutex);
	return (self->playing.empty() || self->preloading.get()) ? getNaN() : self->playing[0].duration();
}

bool Audio::isPlaying() const {
	boost::mutex::scoped_lock l(self->mutex);
	return !self->playing.empty();
}

void Audio::seek(double offset) {
	boost::mutex::scoped_lock l(self->mutex);
	for(boost::ptr_vector<Music>::iterator it = self->playing.begin() ; it != self->playing.end() ; ++it) {
		it->seek(clamp(it->pos() + offset, 0.0, it->duration()));
	}
	pause(false);
}

void Audio::seekPos(double pos) {
	boost::mutex::scoped_lock l(self->mutex);
	for(boost::ptr_vector<Music>::iterator it = self->playing.begin() ; it != self->playing.end() ; ++it) {
		it->seek(pos);
	}
	pause(false);
}

void Audio::pause(bool state) {
	self->paused = state;
}

bool Audio::isPaused() const { return self->paused; }

void Audio::streamFade(std::string track, double fadeLevel) {
	boost::mutex::scoped_lock l(self->mutex);
	Command cmd = { Command::TRACK_FADE, track, fadeLevel };
	self->commands.push_back(cmd);
}

boost::ptr_vector<Analyzer>& Audio::analyzers() { return self->analyzers; }

