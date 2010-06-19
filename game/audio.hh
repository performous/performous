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

struct Sample {
	Sample(std::string, unsigned) {}
};

/** @short High level audio playback API **/
class Audio {
	struct Impl;
	boost::scoped_ptr<Impl> self;
public:
	Audio();
	~Audio();
	boost::ptr_vector<Analyzer>& analyzers() { static boost::ptr_vector<Analyzer> ana; return ana; }
	/// if audio is currently playing
	bool isOpen() const { return false; }
	/** Play a song beginning at startPos (defaults to 0)
	 * @param filename the track filename
	 * @param preview if the song preview is to play
	 * @param fadeTime time to fade
	 * @param startPos starting position
	 */
	void playMusic(std::string const& filename, bool preview = false, double fadeTime = 0.5, double startPos = -0.2);
	/// plays a list of songs
	void playMusic(std::map<std::string,std::string> const& filenames, bool preview = false, double fadeTime = 0.5, double startPos = -0.2);
	/// plays a sample
	void play(Sample const& s, std::string const& volumeSetting);
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
	void toggleSynth(Notes const& notes) { /*m_notes = (m_notes ? NULL : &notes); */} ///< toggles synth playback
	void streamFade(std::string stream_id, double level);
	unsigned int getSR() const { return 48000.0; }
};

