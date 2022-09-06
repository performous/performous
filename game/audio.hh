#pragma once

#include "configuration.hh"
#include "ffmpeg.hh"
#include "notes.hh"
#include "libda/portaudio.hpp"
#include "aubio/aubio.h"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

const unsigned AUDIO_MAX_ANALYZERS = 11;

int PaHostApiNameToHostApiTypeId (const std::string& name);

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
	double m_skew = 0.0; ///< The skew ratio applied to system time (since baseTime)
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

class Analyzer;

struct Device {
	// Init
	const int in, out;
	const double rate;
	const PaDeviceIndex dev;
	portaudio::Stream stream;
	std::vector<Analyzer*> mics;
	Output* outptr;

	Device(int in, int out, double rate, PaDeviceIndex dev);
	/// Start
	void start();
	/// Stop
	void stop();
	/// Callback
	int operator()(float const* input, float* output, std::ptrdiff_t frames);
	/// Returns true if this device is opened for output
	bool isOutput() const { return outptr != nullptr; }
	/// Returns true if this device is assigned to the named channel (mic color or "OUT")
	bool isChannel(std::string const& name) const;
};

extern int getBackend();
class ConfigItem;

/** @short High level audio playback API **/
class Audio {
	friend int getBackend();
		// static because Port audio once for the whole software lifetime
	static portaudio::Init init;
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
	void playMusic(fs::path const& filename, bool preview = false, double fadeTime = 0.5, double startPos = 0.0);
	/** Plays a list of songs **/
	void playMusic(Files const& filenames, bool preview = false, double fadeTime = 0.5, double startPos = 0.0);
	/** Loads/plays/unloads a sample **/
	void loadSample(std::string const& streamId, fs::path const& filename);
	void playSample(std::string const& streamId);
	void unloadSample(std::string const& streamId);
	/** Stops music **/
	void stopMusic();
	/** Fades music out **/
	void fadeout(double time = 1.0);
	/** Get the length of the currently playing song, in seconds. **/
	double getLength() const;
	/**
	 * This methods seek forward in the stream (backwards if
	 * argument is negative), and continues playing.
	 * @param seek_dist number of seconds to seek from current position
	 */
	void seek(double seek_dist);
	/** Seek to specific time **/
	void seekPos(double pos);
	/** Is the music playing (loaded and not at EOF yet, pause doesn't matter) **/
	bool isPlaying() const;
	/** Get the current position. If not known or nothing is playing, NaN is returned. **/
	double getPosition() const;
	void togglePause() { pause(!isPaused()); }
	void pause(bool state = true);
	bool isPaused() const;
	/** Toggle synth playback **/
	void toggleSynth(Notes const&);
	/** Toggle center channel suppressor **/
	void toggleCenterChannelSuppressor();
	/** Adjust volume level of a single track (used for muting incorrectly played instruments). Range 0.0 to 1.0. **/
	void streamFade(std::string track, double volume);
	/** Do a pitch shift - used for guitar whammy bar */
	void streamBend(std::string track, double pitchFactor);
	/** Get sample rate */
	static float getSR() { return 48000.0f; }
	static unsigned aubio_hop_size;
	static unsigned aubio_win_size;
	static std::unique_ptr<aubio_tempo_t, void(*)(aubio_tempo_t*)> aubioTempo;
};

class Music {
struct Track {
	AudioBuffer audioBuffer;
	double fadeLevel = 1.0;
	double pitchFactor = 0.0;
	template <typename... Args> Track(Args&&... args): audioBuffer(std::forward<Args>(args)...) {}
};
	friend class ScreenSongs;
	public:
	std::unordered_map<std::string, std::unique_ptr<Track>> tracks; ///< Audio decoders
	double srate; ///< Sample rate
	std::int64_t m_pos = 0; ///< Current sample position
	bool m_preview;
	class AudioClock m_clock;
	Seconds durationOf(std::int64_t samples) const { return 1.0s * samples / srate / 2.0; }
	float* sampleStartPtr = nullptr;
	float* sampleEndPtr = nullptr;
public:
	bool suppressCenterChannel = false;
	double fadeLevel = 0.0;
	double fadeRate = 0.0;
	using Buffer = std::vector<float>;
	Music(Audio::Files const& files, unsigned int sr, bool preview);
	/// Sums the stream to output sample range, returns true if the stream still has audio left afterwards.
	bool operator()(float* begin, float* end);
	void seek(double time) { m_pos = static_cast<std::int64_t>(time * srate * 2.0); }
	/// Get the current position in seconds
	double pos() const { return m_clock.pos().count(); }
	double duration() const;
	/// Prepare (seek) all tracks to current position, return true when done (nonblocking)
	bool prepare();
	void trackFade(std::string const& name, double fadeLevel);
	void trackPitchBend(std::string const& name, double pitchFactor);
};
