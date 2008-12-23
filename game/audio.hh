#ifndef PERFORMOUS_AUDIO_HH
#define PERFORMOUS_AUDIO_HH

#include <string>

#include "ffmpeg.hh"
#include "notes.hh"
#include <boost/scoped_ptr.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <audio.hpp>

/** @short High level audio playback API **/
class Audio {
  public:
	Audio();
	/** Takes libda devstr and sample rate. Throws if the device fails. **/
	void open(std::string const& pdev, unsigned int rate);
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
	/// gets volume for music playback
	unsigned int getVolumeMusic() { return m_volumeMusic; }
	/// gets volume for preview
	unsigned int getVolumePreview() { return m_volumePreview; }
	/// sets volume for music playback
	void setVolumeMusic(unsigned int volume) { setVolume_internal(m_volumeMusic = volume); }
	/// sets volume for preview
	void setVolumePreview(unsigned int volume) { setVolume_internal(m_volumePreview = volume); }
	void operator()(); ///< Thread runs here, don't call directly
	/// () access
	void operator()(da::pcm_data& areas, da::settings const&);
	/// pauses and unpauses playback
	void togglePause() { m_paused = !m_paused; }
	/// toggles synth playback (F4)
	void toggleSynth(Notes const& notes) { m_notes = (m_notes ? NULL : &notes); }

  private:
	boost::recursive_mutex m_mutex;
	size_t m_crossfade; // Position within m_crossbuf
	std::vector<float> m_crossbuf; // Crossfading buffer
	std::string m_filename;
	double m_volume;
	unsigned m_volumeMusic, m_volumePreview;
	void setVolume_internal(unsigned int volume);
	boost::scoped_ptr<CFfmpeg> m_mpeg;
	bool m_paused;
	bool m_prebuffering;
	Notes const* volatile m_notes;
	da::settings m_rs;
	boost::scoped_ptr<da::playback> m_playback;
};

#endif
