#ifndef PERFORMOUS_AUDIO_HH
#define PERFORMOUS_AUDIO_HH

#include <string>

#include "ffmpeg.hh"
#include "notes.hh"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <audio.hpp>

/// audiostream
/** allows buffering, fading and mixing of audiostreams
 */
struct Stream {
	/// container for the media file
	FFmpeg mpeg;
	/// sample rate
	double srate;
	/// volume
	double volume;
	/// actual "faded volume"
	double fade;
	/// speed of fade effect
	double fadeSpeed;
	/// do buffering or not
	bool prebuffering;
	/// constructor
	Stream(std::string const& filename, unsigned int sr):
	  mpeg(false, true, filename, sr), srate(sr), volume(), fade(1.0), fadeSpeed(), prebuffering(true) {}
	/// fades stream in
	void fadein() { double time = 1.0; fade = 0.0; fadeSpeed = 1.0 / (time * srate); }
	/// fades stream out
	void fadeout() { double time = 1.0; fadeSpeed = - 1.0 / (time * srate); }
	/// crossfades songs
	template <typename RndIt> void playmix(RndIt outbuf, unsigned int maxSamples) {
		if (prebuffering && mpeg.audioQueue.percentage() > 0.9) prebuffering = false;
		if (prebuffering) return;
		std::vector<int16_t> buf;
		mpeg.audioQueue.tryPop(buf, maxSamples);
		if (fade < 0.0) { fade = 0.0; fadeSpeed = 0.0; }
		if (fade > 1.0) { fade = 1.0; fadeSpeed = 0.0; }
		for (size_t i = 0; i < buf.size(); ++i) {
			outbuf[i] += volume * fade * da::conv_from_s16(buf[i]);
			fade += fadeSpeed;
		}
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
	/** Play a song from the beginning
	 * @param filename the track filename
	 * @param preview if the song preview is to play
	 */
	void playMusic(std::string const& filename, bool preview = false);
	/** Play a preview of the song, starting at 30 seconds
	 * @param filename the track filename
	 */
	void playPreview(std::string const& filename) { playMusic(filename, true); }
	/// get pause status
	bool isPaused() { return m_paused; }
	/// stops music
	void stopMusic();
	/// fades music out
	void fadeout();
	/** Get the length of the currently playing song, in seconds. **/
	double getLength() const;
	/**
	 * This methods seek forward in the stream (backwards if
	 * argument is negative), and continues playing.
	 * @param seek_dist number of seconds to seek from current position
	 */
	void seek(double seek_dist);
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

  private:
	mutable boost::recursive_mutex m_mutex;
	void setVolume_internal(unsigned int volume);
	typedef boost::ptr_vector<Stream> Streams;
	Streams m_streams;
	bool m_paused;
	Notes const* volatile m_notes;
	da::settings m_rs;
	boost::scoped_ptr<da::playback> m_playback;
};

#endif
