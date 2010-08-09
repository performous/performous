#pragma once

#include <string>
#include <vector>
#include <map>

#include "ffmpeg.hh"
#include "notes.hh"
#include <boost/date_time.hpp>
#include <boost/shared_ptr.hpp>
#include <libda/mixer.hpp>

using boost::int16_t;
using boost::int64_t;

/// audiostream
/** allows buffering, fading and mixing of audiostreams
 */
struct Stream {
	FFmpeg mpeg; ///< Audio stream
	double srate; ///< Sample rate
	double fade; ///< Fade level
	/// constructor
	Stream(std::string const& filename, unsigned int sr):
	  mpeg(false, true, filename, sr), srate(sr), fade(1.0),
	  m_syncTime(getTime()), m_baseTime(m_syncTime), m_basePosition(0), m_pos() {}
	/// crossfades songs
	bool operator()(da::pcm_data& data) {
		timeSync();  // Keep audio synced
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
	/// Get current position in seconds (with corrections applied)
	double pos() const {
		boost::posix_time::ptime now = getTime();
		const double syncDelay = seconds(now - m_syncTime); // Since last audio buffer
		const double baseDelay = seconds(now - m_baseTime); // Since last time base
		// If more than 100 ms since last sync, freeze time (audio is paused)
		return (syncDelay > 0.1 ? pos_internal() : m_basePosition + baseDelay);
	}
	double duration() const { return mpeg.audioQueue.duration(); }
	bool eof() const { return mpeg.audioQueue.eof(m_pos); }
private:
	static boost::posix_time::ptime getTime() { return boost::posix_time::microsec_clock::universal_time(); }
	double pos_internal() const { return double(m_pos) / srate / 2.0; }
	double seconds(boost::posix_time::time_duration const& d) const { return 1e-6 * d.total_microseconds(); }
	/**
	 * Adjust sync, duration of the audio.
	 * This works by using mostly system time and
	 * syncing it with the audio time every few seconds
	 * (or when something out-of-the-ordinary happens).
	 */
	void timeSync() {
		boost::posix_time::ptime now = getTime();
		const double syncDelay = seconds(now - m_syncTime); // Since last audio buffer
		const double baseDelay = seconds(now - m_baseTime); // Since last time base
		const double delay = std::abs(pos() - pos_internal()); // Prediction difference
		m_syncTime = now;
		// Time to create new reference time?
		// 1. We haven't recieved any audio frames for a while (maybe audio was paused)
		// 2. Time has been running free for several seconds (so let's sync every once in a while)
		// 3. Prediction is radically different from last buffer time (due to e.g. seeking)
		if (syncDelay > 0.1 || baseDelay > 5.0 || delay > 0.2) {
			m_baseTime = now;
			m_basePosition = pos_internal();
		}
	}
	boost::posix_time::ptime m_syncTime; ///< Time of the previous audio timeSync
	boost::posix_time::ptime m_baseTime; ///< A reference time
	double m_basePosition; ///< Position at the base time
	int64_t m_pos; ///< Current sample position
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

