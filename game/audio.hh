#pragma once

#include <string>

#include "configuration.hh"
#include "ffmpeg.hh"
#include "notes.hh"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <libda/audio.hpp>

using boost::int16_t;

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
	 * @param outbuf output buffer
	 * @param maxSamples maximum number of samples
	 */
	template <typename RndIt> void playmix(RndIt outbuf, unsigned int maxSamples) {
		if( offset >= sample.size() ) {
			// we are at the end, nothing to do
			return;
		} else if( sample.size() - offset < maxSamples ) {
			// not enough data in sample
			for (size_t i = offset; i < sample.size(); ++i) {
				outbuf[i-offset] += da::conv_from_s16(sample[i]);
			}
			offset = sample.size();
		} else {
			for (size_t i = offset; i < offset + maxSamples; ++i) {
				outbuf[i-offset] += da::conv_from_s16(sample[i]);
			}
			offset += maxSamples;
		}
	}
};

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
	/// do buffering or not
	bool prebuffering;
	/// constructor
	Stream(std::string const& filename, unsigned int sr, ConfigItem& vol):
	  mpeg(false, true, filename, sr), srate(sr), volume(vol), fade(1.0), fadeSpeed(), prebuffering(true) {}
	/// fades stream in
	void fadein(double time) { fade = fadeSpeed = 1.0 / (time * srate); }
	/// fades stream out
	void fadeout(double time) { fadeSpeed = - 1.0 / (time * srate); }
	/// is the stream fading out
	bool fadingout() {return fadeSpeed < 0;}
	bool consume(double duration) {
		std::vector<int16_t> buf;
		unsigned int samples = srate * duration;
		if( samples == 0 ) return true;
		mpeg.audioQueue.tryPop(buf, samples);
		if( buf.size() == samples ) return true;
		return false;
	}
	/// crossfades songs
	template <typename RndIt> void playmix(RndIt outbuf, unsigned int maxSamples) {
		if (prebuffering && mpeg.audioQueue.percentage() > 0.9) prebuffering = false;
		if (prebuffering) return;
		std::vector<int16_t> buf;
		mpeg.audioQueue.tryPop(buf, maxSamples);
		bool fadeMin = false, fadeMax = false;
		if (fade + maxSamples * fadeSpeed < 0.0) { fadeSpeed = fade / maxSamples; fadeMin = true; }
		if (fade + maxSamples * fadeSpeed > 1.0) { fadeSpeed = (1.0 - fade) / maxSamples; fadeMax = true; }
		double vol = 0.0;
		if (volume.i() > 0) vol = std::pow(10.0, (volume.i() - 100.0) / 100.0 * 2.0);
		for (size_t i = 0; i < buf.size(); ++i) {
			outbuf[i] += vol * fade * da::conv_from_s16(buf[i]);
			fade += fadeSpeed;
		}
		fade += (maxSamples - buf.size()) * fadeSpeed; // Fade continues even if no audio data was received.
		if (fadeMin) { fade = 0.0; fadeSpeed = 0.0; }
		if (fadeMax) { fade = 1.0; fadeSpeed = 0.0; }
		if (buf.size() < maxSamples && !mpeg.audioQueue.eof() && mpeg.position() > 1.0) std::cerr << "Warning: audio decoding too slow (buffer underrun): " << std::endl;
	}
};

/** @short High level audio playback API **/
class Audio {
  public:
	Audio();
	/** Takes libda devstr and sample rate. Throws if the device fails. **/
	void open(std::string const& pdev, std::size_t rate, std::size_t frames);
	/// if audio is currently playing
	bool isOpen() const { return m_playback; }
	/** Play a song beginning at startPos (defaults to 0)
	 * @param filename the track filename
	 * @param preview if the song preview is to play
	 * @param fadeTime time to fade
	 * @param startPos starting position
	 */
	void playMusic(std::string const& filename, bool preview = false, double fadeTime = 0.1, double startPos = 0.0);
	void playMusic(std::vector<std::string> const& filenames, bool preview = false, double fadeTime = 0.1, double startPos = 0.0);
	/** Play a preview of the song, starting at startPos
	 * @param filename the track filename
	 * @param startPos starting position
	 */
	void playPreview(std::string const& filename, double startPos) { playMusic(filename, true, 1.0, startPos); }
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
	void operator()(da::pcm_data& areas, da::settings const&);
	/// pauses and unpauses playback
	void togglePause() { m_paused = !m_paused; }
	/// toggles synth playback (F4)
	void toggleSynth(Notes const& notes) { m_notes = (m_notes ? NULL : &notes); }
	/// if necessary loads and plays a sample
	void playSample(std::string filename);

  private:
	mutable boost::recursive_mutex m_mutex;
	typedef boost::ptr_vector<Stream> Streams;
	Streams m_streams;
	bool m_paused;
	bool m_need_resync;
	Notes const* volatile m_notes;
	da::settings m_rs;
	boost::scoped_ptr<da::playback> m_playback;
	boost::ptr_map<std::string, AudioSample> m_samples;
};

