#pragma once

#include <string>

#include "configuration.hh"
#include "ffmpeg.hh"
#include "notes.hh"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <libda/mixer.hpp>

using boost::int16_t;

#if 0
/// audiosamples for songfiles
struct AudioSample {
	/// sample rate
	double srate;
	/// volume
	double volume;
	/// sample
	std::vector<int16_t> sample;
	/// current position in the sample
	unsigned int offset;
	AudioSample() {}
	/// constructor
	AudioSample(std::string const& filename, unsigned int sr): srate(sr), volume(), sample(), offset() {
		FFmpeg mpeg(false, true, filename, sr);
		while( !mpeg.audioQueue.eof() ) {
			std::vector<int16_t> buffer;
			mpeg.audioQueue.tryPop(buffer, 1024);
			sample.insert(sample.end(), buffer.begin(), buffer.end());
			if( sample.size() > 1024 * 256 ) break; // do not authorized sample > 256k
		}
		std:: cout << "Loading sample " << filename << " done (size: " << sample.size() << ")" << std::endl;
	}
	/// resets audio position
	void reset_position() {
		offset = 0;
	}
	/** plays and mixes samples
	 */
	bool operator()(da::pcm_data& data) {
		std::size_t maxSamples = data.channels * data.frames;
		if( offset >= sample.size() ) return false;
		if( sample.size() - offset < maxSamples ) {
			// not enough data in sample
			for (size_t i = offset; i < sample.size(); ++i) {
				data.rawbuf[i-offset] += da::conv_from_s16(sample[i]);
			}
			offset = sample.size();
		} else {
			for (size_t i = offset; i < offset + maxSamples; ++i) {
				data.rawbuf[i-offset] += da::conv_from_s16(sample[i]);
			}
			offset += maxSamples;
		}
		return true;
	}
};
#endif

/// audiostream
/** allows buffering, fading and mixing of audiostreams
 */
struct Stream {
	/// container for the media file
	FFmpeg mpeg;
	/// sample rate
	double srate;
	/// volume setting to follow
	ConfigItem& volume;
	/// actual "faded volume"
	double fade;
	/// speed of fade effect
	double fadeSpeed;
	/// constructor
	Stream(std::string const& filename, unsigned int sr, ConfigItem& vol):
	  mpeg(false, true, filename, sr), srate(sr), volume(vol), fade(1.0), fadeSpeed(), m_pos() {}
	/// fades stream in
	void fadein(double time) { fade = fadeSpeed = 1.0 / (time * srate); }
	/// fades stream out
	void fadeout(double time) { fadeSpeed = - 1.0 / (time * srate); }
	/// is the stream fading out
	bool fadingout() {return fadeSpeed < 0;}
	/// crossfades songs
	bool operator()(da::pcm_data& data) {
		std::size_t maxSamples = data.channels * data.frames;
		mpeg.audioQueue(data, m_pos);
		bool fadeMin = false, fadeMax = false;
		if (fade + maxSamples * fadeSpeed < 0.0) { fadeSpeed = fade / maxSamples; fadeMin = true; }
		if (fade + maxSamples * fadeSpeed > 1.0) { fadeSpeed = (1.0 - fade) / maxSamples; fadeMax = true; }
		double vol = 0.0;
		if (volume.i() > 0) vol = std::pow(10.0, (volume.i() - 100.0) / 100.0 * 2.0);
		for (size_t i = 0; i < maxSamples; ++i) {
			data.rawbuf[i] *= vol * fade;
			fade += fadeSpeed;
		}
		if (fadeMin) { fade = 0.0; fadeSpeed = 0.0; }
		if (fadeMax) { fade = 1.0; fadeSpeed = 0.0; }
		return !eof();
	}
	void seek(double time) { m_pos = time * srate * 2.0; }
	double pos() const { return double(m_pos) / srate / 2.0; }
	double duration() const { return mpeg.audioQueue.duration(); }
	bool eof() const { return mpeg.audioQueue.eof(m_pos) || (fade == 0.0 && fadeSpeed == 0.0); }
	int64_t m_pos;
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
	void playMusic(std::string const& filename, bool preview = false, double fadeTime = 0.1, double startPos = -0.1);
	/// plays a list of songs
	void playMusic(std::vector<std::string> const& filenames, bool preview = false, double fadeTime = 0.1, double startPos = -0.1);
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
	void togglePause() { m_paused = !m_paused; }
	/// toggles synth playback (F4)
	void toggleSynth(Notes const& notes) { m_notes = (m_notes ? NULL : &notes); }

  private:
	mutable boost::recursive_mutex m_mutex;
	bool m_paused;
	Notes const* volatile m_notes;
	da::settings m_rs;
	da::mixer m_mixer;
	std::vector<boost::shared_ptr<Stream> > m_streams;
};

