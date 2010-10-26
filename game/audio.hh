#pragma once

#include <string>
#include <vector>
#include <map>

#include "ffmpeg.hh"
#include "notes.hh"
#include "pitch.hh"
#include <boost/date_time.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include "libda/portaudio.hpp"

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
	/// Returns true if this device has the given mic color assigned
	bool isMic(std::string mic_id) const {
		for (std::vector<Analyzer*>::const_iterator it = mics.begin(); it != mics.end(); ++it)
			if (*it && (*it)->getId() == mic_id) return true;
		return false;
	}
};


/** @short High level audio playback API **/
class Audio {
	struct Impl;
	boost::scoped_ptr<Impl> self;
public:
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
	void playMusic(std::string const& filename, bool preview = false, double fadeTime = 0.5, double startPos = 0.0);
	/** Plays a list of songs **/
	void playMusic(std::map<std::string,std::string> const& filenames, bool preview = false, double fadeTime = 0.5, double startPos = 0.0);
	/** Loads/plays/unloads a sample **/
	void loadSample(std::string const& streamId, std::string const& filename);
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
	/** Adjust volume level of a single track (used for muting incorrectly played instruments). Range 0.0 to 1.0. **/
	void streamFade(std::string track, double volume);
	/** Do a pitch shift - used for guitar whammy bar */
	void streamBend(std::string track, double pitchFactor);
	/** Get sample rate */
	double getSR() const { return 48000.0; }
};

