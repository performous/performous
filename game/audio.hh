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

/** @short High level audio playback API **/
class Audio {
	struct Impl;
	boost::scoped_ptr<Impl> self;
public:
	Audio();
	~Audio();
	boost::ptr_vector<Analyzer>& analyzers();
	bool isOpen() const;
	/** Play a song beginning at startPos (defaults to 0)
	 * @param filename the track filename
	 * @param preview if the song preview is to play
	 * @param fadeTime time to fade
	 * @param startPos starting position
	 */
	void playMusic(std::string const& filename, bool preview = false, double fadeTime = 0.5, double startPos = 0.0);
	/// plays a list of songs
	void playMusic(std::map<std::string,std::string> const& filenames, bool preview = false, double fadeTime = 0.5, double startPos = 0.0);
	/// loads/plays/unloads a sample
	void loadSample(std::string streamId, std::string filename);
	void playSample(std::string streamId);
	void unloadSample(std::string streamId);
	/// stops music
	void stopMusic();
	/// fades music out
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
	void toggleSynth(Notes const&) { /*m_notes = (m_notes ? NULL : &notes); */} ///< toggles synth playback
	/// Adjust volume level of a single track (used for muting incorrectly played instruments). Range 0.0 to 1.0.
	void streamFade(std::string track, double volume);
	double getSR() const { return 48000.0; }
};

