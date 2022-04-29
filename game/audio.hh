#pragma once

#include "configuration.hh"
#include "ffmpeg.hh"
#include "notes.hh"
#include "pitch.hh"
#include "libda/portaudio.hpp"
#include "aubio/aubio.h"
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

const unsigned AUDIO_MAX_ANALYZERS = 11;

struct Output;

/**
* Advanced audio sync code.
* Produces precise monotonic clock synced to audio output callback (which may suffer of major jitter).
* Uses system clock as timebase but the clock is skewed (made slower or faster) depending on whether
* it is late or early. The clock is also stopped if audio output pauses.
**/
class AudioClock {
	mutable std::mutex m_mutex;
	Time m_baseTime; ///< A reference time (corresponds to m_basePos)
	Seconds m_basePos = 0.0s; ///< A reference position in song
	float m_skew = 0.0; ///< The skew ratio applied to system time (since baseTime)
	std::atomic<Seconds> m_max{ 0.0s }; ///< Maximum output value for the clock (end of the current audio block)
	/// Get the current position (current time via parameter, no locking)
	Seconds pos_internal(Time now) const;
public:
	/**
	* Called from audio callback to keep the clock synced.
	* @param audioPos the current position in the song
	* @param length the duration of the current audio block
	*/
	void timeSync(Seconds audioPos, Seconds length);
	/// Get the current position in seconds
	Seconds pos() const;
};

struct Device {
	// Init
	const unsigned int in, out;
	const float rate;
	const unsigned int dev;
	portaudio::Stream stream;
	std::vector<Analyzer*> mics;
	Output* outptr;

	Device(unsigned int in, unsigned int out, float rate, unsigned int dev);
	/// Start
	void start();
	/// Stop
	void stop();
	/// Callback
	int operator()(float const* input, float* output, unsigned long frames);
	/// Returns true if this device is opened for output
	bool isOutput() const { return outptr != nullptr; }
	/// Returns true if this device is assigned to the named channel (mic color or "OUT")
	bool isChannel(std::string const& name) const {
		if (name == "OUT") return isOutput();
		for (auto const& m: mics) if (m && m->getId() == name) return true;
		return false;
	}
};

extern int getBackend();
class ConfigItem;

/** @short High level audio playback API **/
class Audio {
	friend int getBackend();
	struct Impl;
	std::unique_ptr<Impl> self;
	friend class ScreenSongs;
	friend class Music;
	static std::recursive_mutex aubio_mutex;
public:
	typedef std::map<std::string, fs::path> Files;
	static ConfigItem& backendConfig();
	Audio();
	~Audio();
	void restart();
	void close();
	std::deque<Analyzer>& analyzers();
	std::deque<Device>& devices();
	bool isOpen() const;
	bool hasPlayback() const;
	/** Play a song beginning at startPos (defaults to 0)
	 * @param filename the track filename
	 * @param preview if the song preview is to play
	 * @param fadeTime time to fade
	 * @param startPos starting position
	 */
	void playMusic(fs::path const& filename, bool preview = false, float fadeTime = 0.5f, float startPos = 0.0f);
	/** Plays a list of songs **/
	void playMusic(Files const& filenames, bool preview = false, float fadeTime = 0.5f, float startPos = 0.0f);
	/** Loads/plays/unloads a sample **/
	void loadSample(std::string const& streamId, fs::path const& filename);
	void playSample(std::string const& streamId);
	void unloadSample(std::string const& streamId);
	/** Stops music **/
	void stopMusic();
	/** Fades music out **/
	void fadeout(float time = 1.0f);
	/** Get the length of the currently playing song, in seconds. **/
	float getLength() const;
	/**
	 * This methods seek forward in the stream (backwards if
	 * argument is negative), and continues playing.
	 * @param seek_dist number of seconds to seek from current position
	 */
	void seek(float seek_dist);
	/** Seek to specific time **/
	void seekPos(float pos);
	/** Is the music playing (loaded and not at EOF yet, pause doesn't matter) **/
	bool isPlaying() const;
	/** Get the current position. If not known or nothing is playing, NaN is returned. **/
	float getPosition() const;
	void togglePause() { pause(!isPaused()); }
	void pause(bool state = true);
	bool isPaused() const;
	/** Toggle synth playback **/
	void toggleSynth(Notes const&);
	/** Toggle center channel suppressor **/
	void toggleCenterChannelSuppressor();
	/** Adjust volume level of a single track (used for muting incorrectly played instruments). Range 0.0 to 1.0. **/
	void streamFade(std::string track, float volume);
	/** Do a pitch shift - used for guitar whammy bar */
	void streamBend(std::string track, float pitchFactor);
	/** Get sample rate */
	static float getSR() { return 48000.0f; }
	static unsigned aubio_hop_size;
	static unsigned aubio_win_size;
	static std::unique_ptr<aubio_tempo_t, void(*)(aubio_tempo_t*)> aubioTempo;
};

class Music {
struct Track {
	AudioBuffer audioBuffer;
	float fadeLevel = 1.0f;
	float pitchFactor = 0.0f;
	template <typename... Args> Track(Args&&... args): audioBuffer(std::forward<Args>(args)...) {}
};	
	friend class ScreenSongs;
	public:
	std::unordered_map<std::string, std::unique_ptr<Track>> tracks; ///< Audio decoders
	float srate; ///< Sample rate
	int64_t m_pos = 0; ///< Current sample position
	bool m_preview;
	class AudioClock m_clock;
	Seconds durationOf(int64_t samples) const { return 1.0s * samples / srate / 2.0f; }
	float* sampleStartPtr = nullptr;
	float* sampleEndPtr = nullptr;
public:
	bool suppressCenterChannel = false;
	float fadeLevel = 0.0f;
	float fadeRate = 0.0f;
	using Buffer = std::vector<float>;
	Music(Audio::Files const& files, unsigned int sr, bool preview);
	/// Sums the stream to output sample range, returns true if the stream still has audio left afterwards.
	bool operator()(float* begin, float* end);
	void seek(float time) { m_pos = time * srate * 2.0f; }
	/// Get the current position in seconds
	float pos() const { return m_clock.pos().count(); }
	float duration() const;
	/// Prepare (seek) all tracks to current position, return true when done (nonblocking)
	bool prepare();
	void trackFade(std::string const& name, float fadeLevel);
	void trackPitchBend(std::string const& name, float pitchFactor);
};
