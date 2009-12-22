#pragma once

#include <string>
#include <vector>
#include <map>

#include "ffmpeg.hh"
#include "notes.hh"
#include <boost/shared_ptr.hpp>
#include <libda/mixer.hpp>

using boost::int16_t;
using boost::int64_t;

/// audiostream
/** allows buffering, fading and mixing of audiostreams
 */
struct Stream {
	/// container for the media file
	FFmpeg mpeg;
	/// sample rate
	double srate;
	/// constructor
	Stream(std::string const& filename, unsigned int sr):
	  mpeg(false, true, filename, sr), srate(sr), m_pos(), fade(1.0) {}
	/// crossfades songs
	bool operator()(da::pcm_data& data) {
		if (fade == 1.0) {
			mpeg.audioQueue(data, m_pos);
		} else {
			std::vector<da::sample_t> tmp(data.samples());
			da::pcm_data dtmp(&tmp[0], data.frames, data.channels, data.rate);
			mpeg.audioQueue(dtmp, m_pos);
			for (std::size_t i = 0, s = data.samples(); i < s; ++i) {
				data[i] += dtmp[i] * fade;
			}
		}
		return !eof();
	}
	void seek(double time) { m_pos = time * srate * 2.0; }
	double pos() const { return double(m_pos) / srate / 2.0; }
	double duration() const { return mpeg.audioQueue.duration(); }
	bool eof() const { return mpeg.audioQueue.eof(m_pos); }
	int64_t m_pos;
	double fade;
};

struct Sample {
	Sample(std::string const& filename, unsigned int sr): mpeg(new FFmpeg(false, true, filename, sr)) {}
	boost::shared_ptr<FFmpeg> mpeg;
};

/** @short High level audio playback API **/
class Audio {
  public:
	Audio();
	/** Takes libda devstr and sample rate. Throws if the device fails. **/
	void open(std::string const& pdev, std::size_t rate, std::size_t frames);
	/// if audio is currently playing
	bool isOpen() const { return m_mixer.started(); }
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
	/// get pause status
	bool isPaused() { return m_paused; }
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
	void operator()(); ///< Thread runs here, don't call directly
	/// callback for the audio backend (libda)
	bool operator()(da::pcm_data& areas);
	/// pauses and unpauses playback
	void togglePause() { pause(!m_paused); }
	void pause(bool state = true);
	/// toggles synth playback (F4)
	void toggleSynth(Notes const& notes) { m_notes = (m_notes ? NULL : &notes); }
	void streamFade(std::string stream_id, double level);
	unsigned int getSR() const { return m_rs.rate(); }
  private:
	bool m_paused;
	Notes const* volatile m_notes;
	da::settings m_rs;
	da::volume m_volume;
	std::string m_volumeSetting;
	da::mixer m_mixer;
	std::map<std::string,boost::shared_ptr<Stream> > m_streams;
};

