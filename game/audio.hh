#pragma once

#include <boost/date_time.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <map>
#include <string>
#include <vector>

#include "ffmpeg.hh"
#include "libda/portaudio.hpp"
#include "notes.hh"
#include "pitch.hh"
#include "util.hh"
#include "configuration.hh"

const unsigned AUDIO_MAX_ANALYZERS = 11;

struct Output;

struct Device {
	// Init
	const unsigned int in, out;
	const double rate;
	const unsigned int dev;
	portaudio::Stream stream;
	std::vector<Analyzer*> mics;
	Output* outptr;

	Device(unsigned int in, unsigned int out, double rate, unsigned int dev);
	/// Start
	void start();
	/// Callback
	int operator()(void const* input, void* output, unsigned long frames, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags);
	/// Returns true if this device is opened for output
	bool isOutput() const { return outptr != NULL; }
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
	boost::scoped_ptr<Impl> self;
public:
	typedef std::map<std::string, fs::path> Files;
	static ConfigItem& backendConfig();
	Audio();
	~Audio();
	void restart();
	void close();
	boost::ptr_vector<Analyzer>& analyzers();
	boost::ptr_vector<Device>& devices();
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
	double getSR() const { return 48000.0; }
};